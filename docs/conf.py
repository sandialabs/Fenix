# Configuration file for the Sphinx documentation builder.

from docutils import nodes
from docutils.parsers.rst import Directive

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

# -- Custom directives -------------------------------------------------------

class OperationBadge(Directive):
    """Directive for operation type badges (Local/Collective/Custom)"""
    has_content = True
    required_arguments = 0
    optional_arguments = 1
    final_argument_whitespace = True

    def run(self):
        import shlex

        # Get the argument text - it comes as a single string if final_argument_whitespace=True
        if self.arguments:
            arg_line = self.arguments[0]
        elif self.content:
            arg_line = ' '.join(self.content)
        else:
            arg_line = ''

        result = []

        if arg_line:
            # Use shlex to properly parse quoted strings
            try:
                badges = shlex.split(arg_line)
            except ValueError:
                # Fall back to simple split if shlex fails
                badges = arg_line.split()

            for badge_text in badges:
                operation_type = badge_text.lower()

                # Determine CSS class and display text
                if operation_type in ['local', 'collective']:
                    css_class = operation_type
                    text = operation_type.capitalize()
                else:
                    # Custom badge text
                    css_class = ''
                    text = badge_text

                html = f'<span class="operation-badge operation-badge-inline {css_class}">{text}</span>'
                node = nodes.raw('', html, format='html')
                result.append(node)

        return result

def setup(app):
    app.add_directive('operation', OperationBadge)

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# -- Options for HTML output -------------------------------------------------

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
html_css_files = ['css/custom.css']
html_js_files = ['js/custom.js']

# ReadTheDocs theme options
html_theme_options = {
    'navigation_depth': 4,
    'collapse_navigation': False,  # Keep sidebar sections expanded
    'sticky_navigation': True,
    'includehidden': True,
    'titles_only': False,
}

# -- Extension configuration -------------------------------------------------

# C domain as primary
primary_domain = 'c'
highlight_language = 'c'

# Don't add C/C++ domain function signatures to toctrees automatically
# This prevents function directives from creating sub-entries in the sidebar
toc_object_entries = False

# Master document
master_doc = 'index'
