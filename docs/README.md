# Fenix Sphinx Documentation

This directory contains the Sphinx/RST documentation for Fenix, migrated from Doxygen.

## Building the Documentation

### Prerequisites

```bash
pip install sphinx sphinx_rtd_theme
```

### Build

```bash
sphinx-build -b html docs docs/_build/html
```

Or with warnings as errors:

```bash
sphinx-build -W -b html docs docs/_build/html
```

### View

Open `docs/_build/html/index.html` in a browser.

## Structure

- `introduction.rst` - Project overview
- `guides/` - Conceptual documentation (process recovery, data recovery, IMR policy)
- `api/c-api/` - C API reference
- `api/cpp-api/` - C++ API reference
- `examples/` - Example programs overview

## Migration Notes

This documentation was migrated from Doxygen to native Sphinx/RST format without using
conversion tools like Breathe or Doxysphinx. All API documentation is manually written
using Sphinx C domain directives.

Cross-references use:
- `:c:func:`Fenix_Init`` for C functions
- `:c:type:`Fenix_Rank_role`` for C types
- `:c:macro:`FENIX_SUCCESS`` for macros
- `:cpp:func:` for C++ functions
- `:doc:`/guides/process-recovery`` for documents
