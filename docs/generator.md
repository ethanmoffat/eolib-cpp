# Code generator

`eolib-protocol-gen` converts the eo-protocol XML files into the C++ protocol code that's compiled into eolib. It runs automatically as part of the build, so you'll only need to know how it works if you're changing what gets generated. For an overview of the library as a whole, see [Architecture](architecture.md).

The generator follows [eolib-java](https://www.github.com/cirras/eolib-java)'s code generator closely, since that's the reference implementation for the protocol rules. Where the generated API differs, it follows [eolib-dotnet](https://www.github.com/ethanmoffat/eolib-dotnet) and [eolib-go](https://www.github.com/ethanmoffat/eolib-go).

## How it runs

The generator is a small command-line program:

```
eolib-protocol-gen --input <xml dir> --output <dir> [--stamp <file>] [--mode protocol|test-properties]
```

`main.cpp` runs the same steps for every build:

1. Load every `protocol.xml` file under the input directory into an in-memory model (`LoadProtocolFiles`).
2. Build a registry of every type defined across all of the files (`TypeRegistry`).
3. Generate the output files. In `protocol` mode (the default) this is the library code, from `GenerateProtocol`. In `test-properties` mode it's a single test support file, from `GenerateCapturedPacketProperties`.
4. Write each output file, but only if its content changed, then touch the stamp file.

Each step only uses the steps before it. The model doesn't know about types, and neither the model nor the types know anything about C++ code.

Any problem with the protocol files, such as an unknown type, a misplaced element or a duplicate name, is reported by throwing `GeneratorError` with the file and type name in the message. `main.cpp` prints the message and exits with an error, which fails the build.

## Files

All of the generator's code is in `generator/src`, in the `eolib::generator` namespace. The sources are built into a static library (`eolib_protocol_gen_lib`) and the `eolib-protocol-gen` executable, so the generator tests can link the library and call the generator functions directly.

| File | Contents |
|---|---|
| `main.cpp` | Command-line parsing and the steps above |
| `model.hpp/.cpp` | The protocol model and the XML loading code |
| `types.hpp/.cpp` | `Type` and `TypeRegistry` |
| `emitter.hpp/.cpp` | Generates the library code |
| `property_emitter.hpp/.cpp` | Generates the captured packet test support code |
| `doc_comment.hpp/.cpp` | `DocComment`, which builds the text of generated documentation comments |
| `code_writer.hpp` | `CodeWriter`, which builds up generated code line by line |
| `names.hpp/.cpp` | Naming and escaping helpers |
| `errors.hpp` | `GeneratorError` |

## Model

`model.hpp` defines plain structs that mirror the XML. There's no behavior in them beyond two helpers on `ProtocolFile` for its namespace and include path.

```
ProtocolFile
  enums:   ProtocolEnum   -> values: ProtocolEnumValue
  structs: ProtocolStruct -> instructions: Instruction
  packets: ProtocolPacket -> instructions: Instruction

Instruction (kind = Field, Array, Length, Dummy, Switch, Chunked or Break)
  Switch:  cases: ProtocolCase -> instructions: Instruction
  Chunked: instructions: Instruction
```

The XML elements that can appear inside a struct, packet or case are all represented by a single `Instruction` struct. Its `kind` says which element it came from, and only the members that apply to that element are filled in. For example, `offset` is only used for `<length>`, and `cases` is only used for `<switch>`. This keeps the model a direct copy of the XML, and code that handles instructions switches on `kind` instead of using a class hierarchy.

`model.cpp` reads the XML with pugixml. Before reading the elements, it runs a pre-pass that turns XML comments (`<!-- ... -->`) into `<comment>` elements, so they can be read the same way as comments that were written as elements. An XML comment documents the element after it, or its parent element if nothing follows it. This is the same rule used by eolib-dotnet and eolib-go.

Files are loaded in sorted order so that the generated output is the same on every machine.

## Types

The model only has type names as strings, such as `"short"`, `"Coords"` or `"bool:short"`. `TypeRegistry` turns those names into `Type` objects with everything the emitters need to know:

- The kind of type: integer, bool, string, blob, enum or struct.
- The fixed size in bytes, if the type always has the same size.
- Whether the type is bounded, meaning the reader can tell where it ends without a length or a break byte.
- The underlying type, for enums and for references like `bool:short`. `SerializationType()` returns the type that's actually read and written.
- Pointers back to the model definition and the file that defines it, for enums and structs.

The registry is built in two stages. The constructor indexes the name of every enum and struct in every file, so a type can be used in a different file from the one that defines it. `Type` objects are then created the first time they're requested by `Get` and kept for the rest of the run, so the emitters can hold on to `Type` pointers. Calculating the size of a struct means getting the types of its fields, so the registry tracks which structs are in progress and reports an error if a struct contains itself.

## Library code (`emitter.cpp`)

`emitter.cpp` has two classes. `ProtocolGenerator` produces the output files, and `ObjectGenerator` produces the code for one class within a file.

### ProtocolGenerator

`GenerateProtocol` creates a `ProtocolGenerator` and calls `Generate()`, which goes through the protocol files one at a time and generates:

1. `enums.hpp/.cpp`, with `GenerateEnums`. Enums are simple enough that this function writes them directly.
2. `structs.hpp/.cpp`, with `GenerateStructs`.
3. `packets.hpp/.cpp`, with `GeneratePackets`, and `packet_factory.hpp/.cpp`, with `GeneratePacketFactory`.
4. The umbrella headers, with `GenerateUmbrella` and `GenerateRootUmbrella`.

`FileLayout` (in `emitter.hpp`) decides which of these files exist for a protocol file. The CMake build makes the same decision when it configures the project, so the two need to stay in sync.

`GenerateStructs` and `GeneratePackets` work the same way. They write the start of the header and source files (banner, includes and namespace), then create an `ObjectGenerator` for each struct or packet and add its code to both files. The includes are worked out from the types each object uses. Structs are sorted so that a struct is declared before any struct in the same file that uses it, since C++ requires a complete type for a member.

### ObjectGenerator

`ObjectGenerator` generates a single class: a struct, a packet or a switch case. It's used in two calls:

```cpp
ObjectGenerator generator(types_, file, protocol_struct->name, protocol_struct->name, Context{});
generator.GenerateInstructions(protocol_struct->instructions);
generator.Finish(protocol_struct->comment, std::nullopt, header, source);
```

`GenerateInstructions` goes through the instructions once, in order. For each one, `GenerateInstruction` checks the rules about what can follow what, then calls a function for the instruction's kind: `GenerateField`, `GenerateArray`, `GenerateLength`, `GenerateDummy`, `GenerateSwitch`, `GenerateChunked` or `GenerateBreak`.

Each instruction adds to several parts of the class at the same time, so the generator keeps a separate `CodeWriter` or list for each part:

| Member | Holds |
|---|---|
| `members_` | Field declarations, constants and nested case classes |
| `serialize_` | The body of `Serialize` |
| `deserialize_` | The body of `Deserialize` |
| `equals_` | The fields compared by `operator==` |
| `to_string_` | The fields printed by `ToString` |
| `nested_definitions_` | The `.cpp` code for nested case classes |

Most of the `Generate*` functions follow the same steps. They validate the instruction, declare the member (if there is one) along with its documentation, and then add the serialize and deserialize code. Lower-level helpers are shared between them: `WriteMethod` and `ReadMethod` choose the `EoWriter` or `EoReader` function for a type, and `WriteStatement` and `ReadExpression` build the code that calls them.

`Context` holds what the generator needs to remember while it works through the instructions:

- Whether the current instructions are inside a `<chunked>` section.
- Whether an optional field, a dummy, or an array without a length has been reached, since each of these limits what can come after it.
- The fields declared so far (`accessible_fields`), so a `<switch>` can look up the field it switches on.
- Which `<length>` fields have been declared, and which field uses each one.

Finally, `Finish` checks that every length field was used by another field, then writes the class. The declaration and documentation go to the header writer, and the `Serialize`, `Deserialize`, `operator==` and `ToString` definitions go to the source writer.

### Switches

Switches are where `ObjectGenerator` calls itself. `GenerateSwitch` handles each case:

- A case with no instructions only adds `case` labels to the serialize and deserialize code.
- A case with instructions gets a new `ObjectGenerator` for a nested class (for example `ReplyCodeDataOk`). The new generator starts with a copy of the parent's `Context` flags and generates its own instructions. Its `Finish` output is added to the parent's members, and its flags are merged back into the parent's `Context` afterwards.

After all cases have been generated, the switch adds the `std::variant` type alias and the `<field>_data` member, with `std::monostate` as the first alternative.

### Documentation comments

Documentation comments are built with `DocComment` (in `doc_comment.cpp`). It collects paragraphs and notes, and writes the notes as a `@note` list after the paragraphs. It also builds the documentation for comments that don't belong to a member. A comment on a dummy, for example, becomes a note on the class that starts with a description of the dummy ("The dummy byte (always 255): ..."), and comments in empty switch cases are added to the documentation of the switch's data member.

`DocComment` passes all text through `DocText` when it's added, which escapes characters that Doxygen would otherwise treat as commands.

## Test support code (`property_emitter.cpp`)

The captured packet tests compare every field of a deserialized packet to the values recorded in eo-captured-packets, and they also need to build packets from those values. Writing that code by hand for hundreds of packets isn't practical, so it's generated too.

`PropertyGenerator` walks the same model as `ObjectGenerator`, but it's a separate, much smaller class. For each packet, and each struct used by a packet, it generates a `FromProperties<T>` function that creates the object and sets its fields from the JSON properties. The output is a single file, `captured_packet_properties.cpp`, which is only compiled into the tests.

## Making changes

When you change the generator, it helps to compare the generated code before and after the change. The generated code for a build is in the `generated` folder of the build directory (`build/release` by default):

```bash
./build-linux.sh --no-install --offline --test
cp -r build/release/generated /tmp/generated-before
# make the change, then rebuild
./build-linux.sh --no-install --offline --test
diff -r /tmp/generated-before build/release/generated
```

Every change to the generator should come with tests in `tests/generator`. `generator_test_utils.hpp` has a `Generate` helper that runs the generator on an XML snippet and returns the generated files, so most tests are a few lines of XML and a check on the output. Changes that affect behavior should also have a test in `tests/protocol/generated_protocol_test.cpp`, which runs the real generated code.

> ⚠️ If a change adds or removes a kind of generated file, update `FileLayout` in `emitter.hpp` and the matching logic in `cmake/EolibGenerate.cmake`. Otherwise the build won't know about the new files.
