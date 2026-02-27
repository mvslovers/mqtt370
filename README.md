# MQTT370

**MQTT370** is an MQTT client, broker, and CLI for IBM MVS 3.8j, originally created by Michael Dean Rayborn. It provides MQTT 5.0 messaging capabilities for applications running on Hercules-emulated MVS systems.

This project is maintained as part of the [mvslovers](https://github.com/mvslovers) community.

## Components

- **client/** — MQTT client library (connect, publish, subscribe)
- **broker/** — MQTT message broker with Lua-based configuration
- **cli/** — Command-line interface for MQTT operations
- **utility/** — Shared utility functions (encoding, networking, etc.)

## Building

### Prerequisites

- Cross-compilation environment for MVS 3.8j
- `c2asm370` compiler (GCC-based, targeting S/370)
- `mvsasm` assembler script
- CRENT370 libraries by Mike Rayborn

### Clone

```bash
git clone --recursive https://github.com/mvslovers/mqtt370.git
cd mqtt370
```

### Configuration

Copy `.env.example` to `.env` and set your MVS host credentials and dataset names:

```bash
cp .env.example .env
# edit .env with your values
```

### Compile

```bash
make            # build all components
make clean      # clean all components
```

To build a single component:

```bash
make -C client
make -C broker
make -C cli
make -C utility
```

The build pipeline is: C source → `c2asm370` → S/370 Assembly (.s) → `mvsasm` → Object decks (.o) → MVS datasets.

## Project Structure

```
client/src/     MQTT client library source files
broker/src/     MQTT broker source files
cli/src/        CLI tool source files
utility/src/    Shared utility source files
include/        Header files (shared across all components)
contrib/        Git submodules (SDK headers for dependencies)
scripts/        Build helper scripts
doc/            Documentation
```

## Acknowledgments

This project was created by **Michael Dean Rayborn**, whose extensive work on MVS 3.8j tooling forms the foundation of the entire MVS open-source ecosystem.

## License

This project is licensed under the [MIT License](LICENSE).
