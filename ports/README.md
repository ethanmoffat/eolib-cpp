# vcpkg port

`ports/eolib` is a vcpkg [overlay port](https://learn.microsoft.com/vcpkg/concepts/overlay-ports).

## Using the overlay port

```sh
vcpkg install eolib --overlay-ports=path/to/eolib-cpp/ports
```

In manifest mode, add `"eolib"` to your `vcpkg.json` dependencies, and add the overlay path to
`vcpkg-configuration.json` (`"overlay-ports": ["path/to/eolib-cpp/ports"]`). Then consume it with CMake:

```cmake
find_package(eolib CONFIG REQUIRED)
target_link_libraries(main PRIVATE eolib::eolib)
```

## Updating the port after a release

The port downloads the release source archive, `eolib-<version>-src.tar.gz`. That archive includes the
`eo-protocol` submodule, which GitHub's auto-generated archives do not.

1. Publish the release by pushing the `v<version>` tag, which runs `release.yml`.
2. Set `version-semver` in `vcpkg.json` to the released version, without the leading `v`.
3. Set `SHA512` in `portfile.cmake` to the contents of the release's `eolib-<version>-src.tar.gz.sha512` asset.
4. Test it with `vcpkg install eolib --overlay-ports=ports`.

Until a release exists, `SHA512 0` is a placeholder. To test before a release, build the source archive
the way `release.yml` does, then temporarily point `URLS` at `file:///path/to/archive` with its real SHA512.

## Registry

Once the API is stable, the port will move to a custom registry (`ethanmoffat/vcpkg-registry`), which
consumers reference from `vcpkg-configuration.json`. Submitting to the curated vcpkg registry is optional
after that.

## Known limitations

- Cross-compiling triplets (for example `arm64-linux` on an x64 host) build the protocol generator for the
  host with `ExternalProject`, which downloads pugixml with FetchContent. That fails when vcpkg builds
  without network access. Native triplets use vcpkg's `pugixml`.
