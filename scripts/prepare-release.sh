#!/usr/bin/env bash
#
# Updates every file that references the version for a new release, then runs validate-release.sh:
#   - CMakeLists.txt: project() VERSION and EOLIB_VERSION_SUFFIX
#   - CHANGELOG.md: moves the [Unreleased] entries into a new dated section, and updates the comparison links
#   - README.md, docs/ and tests/consumer: release archive URLs and find_package(eolib MAJOR.MINOR) versions
#
# Optionally commits the changes and creates the annotated release tag. Nothing is pushed.

set -u

SCRIPT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
REPO_ROOT="$(dirname "${SCRIPT_ROOT}")"
REPO_URL="https://github.com/ethanmoffat/eolib-cpp"
CHANGELOG="${REPO_ROOT}/CHANGELOG.md"

VERSION=""
RELEASE_DATE="$(date +%Y-%m-%d)"
COMMIT=false
TAG=false

usage() {
    echo "Usage:"
    echo "  prepare-release.sh <version> [options]"
    echo ""
    echo "  <version>            Version to release, with or without the leading 'v' (e.g. 0.2.0-beta.1, 1.0.0)"
    echo "  --date <YYYY-MM-DD>  Release date for the changelog [default: today]"
    echo "  --commit             Commit the changes"
    echo "  --tag                Commit the changes and create the annotated tag v<version>"
    echo "  -h --help            Display this message"
    echo ""
    echo "Push master first, then the tag, which starts the release workflow."
}

while [ $# -gt 0 ]; do
    case "$1" in
        --date)
            RELEASE_DATE="${2:-}"
            shift
            ;;
        --commit)
            COMMIT=true
            ;;
        --tag)
            COMMIT=true
            TAG=true
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        -*)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
        *)
            VERSION="${1#v}"
            ;;
    esac
    shift
done

if [ -z "${VERSION}" ]; then
    usage
    exit 1
fi

if [[ ! "${VERSION}" =~ ^((0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*))(-((beta|rc)\.[1-9][0-9]*))?$ ]]; then
    echo "Version ${VERSION} must be MAJOR.MINOR.PATCH with an optional -beta.N or -rc.N suffix"
    exit 1
fi
NUMERIC_VERSION="${BASH_REMATCH[1]}"
MAJOR_MINOR="${BASH_REMATCH[2]}.${BASH_REMATCH[3]}"
SUFFIX="${BASH_REMATCH[6]}"

if [[ ! "${RELEASE_DATE}" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]]; then
    echo "Release date ${RELEASE_DATE} must be YYYY-MM-DD"
    exit 1
fi

if ${COMMIT} && [ -n "$(git -C "${REPO_ROOT}" status --porcelain --untracked-files=no)" ]; then
    echo "The working tree has uncommitted changes. Commit or stash them before using --commit or --tag."
    exit 1
fi

if ${TAG} && git -C "${REPO_ROOT}" rev-parse --verify --quiet "refs/tags/v${VERSION}" > /dev/null; then
    echo "Tag v${VERSION} already exists."
    exit 1
fi

# Check the changelog before changing anything
#
escaped_version="${VERSION//./\\.}"
has_section=false
if grep -qE "^## \[${escaped_version}\]" "${CHANGELOG}"; then
    has_section=true
fi
unreleased_entries="$(awk '
    /^## \[Unreleased\]/ { in_section = 1; next }
    in_section && /^## \[/ { exit }
    in_section && /^- / { count++ }
    END { print count + 0 }' "${CHANGELOG}")"
if ! ${has_section} && [ "${unreleased_entries}" -eq 0 ]; then
    echo "CHANGELOG.md has no [Unreleased] entries to release. Add them first."
    exit 1
fi

# CMakeLists.txt
#
sed -i.bak -E \
    -e "s/^    VERSION [0-9]+\.[0-9]+\.[0-9]+$/    VERSION ${NUMERIC_VERSION}/" \
    -e "s/^set\(EOLIB_VERSION_SUFFIX \".*\"\)$/set(EOLIB_VERSION_SUFFIX \"${SUFFIX}\")/" \
    "${REPO_ROOT}/CMakeLists.txt"
rm -f "${REPO_ROOT}/CMakeLists.txt.bak"
echo "Updated CMakeLists.txt to ${NUMERIC_VERSION}${SUFFIX:+ (suffix ${SUFFIX})}"

# CHANGELOG.md
#
if ${has_section}; then
    echo "CHANGELOG.md already has a section for ${VERSION}; leaving its entries as they are"
else
    # Insert the new heading below [Unreleased], so its entries become the new version's entries.
    awk -v heading="## [${VERSION}] - ${RELEASE_DATE}" '
        { print }
        /^## \[Unreleased\]$/ && !done { print ""; print heading; done = 1 }' "${CHANGELOG}" > "${CHANGELOG}.tmp"
    mv "${CHANGELOG}.tmp" "${CHANGELOG}"
    echo "Moved ${unreleased_entries} [Unreleased] entries to ## [${VERSION}] - ${RELEASE_DATE}"
fi

# Point [Unreleased] at the new tag, and add the new version's link below it.
awk -v version="${VERSION}" -v url="${REPO_URL}" '
    /^\[Unreleased\]:/ {
        print "[Unreleased]: " url "/compare/v" version "...HEAD"
        if (!has_link) { print "[" version "]: " url "/releases/tag/v" version }
        next
    }
    { print }' has_link="$(grep -cF "[${VERSION}]: " "${CHANGELOG}")" "${CHANGELOG}" > "${CHANGELOG}.tmp"
mv "${CHANGELOG}.tmp" "${CHANGELOG}"
echo "Updated CHANGELOG.md links"

# Documentation and the consumer test: release archive URLs and find_package versions
#
while IFS= read -r file; do
    sed -i.bak -E \
        -e "s#releases/download/v[0-9]+\.[0-9]+\.[0-9]+(-[a-z]+\.[0-9]+)?/eolib-[0-9]+\.[0-9]+\.[0-9]+(-[a-z]+\.[0-9]+)?-src#releases/download/v${VERSION}/eolib-${VERSION}-src#g" \
        -e "s#eolib-[0-9]+\.[0-9]+\.[0-9]+(-[a-z]+\.[0-9]+)?-src\.tar\.gz\.sha512#eolib-${VERSION}-src.tar.gz.sha512#g" \
        -e "s#find_package\(eolib [0-9]+\.[0-9]+ #find_package(eolib ${MAJOR_MINOR} #g" \
        "${file}"
    rm -f "${file}.bak"
done < <(cd "${REPO_ROOT}" && git ls-files README.md 'docs/*.md' 'tests/consumer/*' | sed "s#^#${REPO_ROOT}/#")
echo "Updated documentation and consumer test versions"

echo ""
git -C "${REPO_ROOT}" --no-pager diff --stat
echo ""

# The release commit doesn't exist yet, so check against the current branch instead of origin/master.
if ! "${SCRIPT_ROOT}/validate-release.sh" "${VERSION}" --branch HEAD; then
    echo ""
    echo "Fix the problems above, then re-run this script or validate-release.sh."
    exit 1
fi

if ${COMMIT}; then
    git -C "${REPO_ROOT}" add -u
    git -C "${REPO_ROOT}" commit -q -m "Release ${VERSION}" || exit 1
    echo "Committed \"Release ${VERSION}\""
fi

if ${TAG}; then
    git -C "${REPO_ROOT}" tag -a "v${VERSION}" -m "eolib v${VERSION}" || exit 1
    echo "Created tag v${VERSION}"
fi

echo ""
echo "Next: push master (git push origin master), wait for CI, then push the tag (git push origin v${VERSION})."
