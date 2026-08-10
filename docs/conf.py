# Configuration file for the Sphinx documentation builder.

# -- Project information -----------------------------------------------------

project = 'Fenix'
copyright = '2016-2026, Rutgers University and Sandia Corporation'
author = 'Fenix Development Team'
version = '2.x'
release = '2.x'

# -- General configuration ---------------------------------------------------

extensions = [
    'sphinx.ext.intersphinx',
    'sphinx.ext.viewcode',
    'sphinx.ext.todo',
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# -- Options for HTML output -------------------------------------------------

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
html_css_files = ['css/custom.css']

# -- Extension configuration -------------------------------------------------

# C domain as primary
primary_domain = 'c'
highlight_language = 'c'

# Master document
master_doc = 'index'
