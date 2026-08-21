#!/usr/bin/env python3
"""
Compare lcov coverage reports and generate a markdown report for PR comments.
"""

import argparse
import re
import sys
from pathlib import Path
from typing import Dict, Tuple


def parse_lcov_file(lcov_file: Path) -> Dict[str, Tuple[int, int]]:
    """
    Parse an lcov info file and extract coverage data.

    Returns:
        Dict mapping file paths to (lines_hit, lines_found) tuples
    """
    coverage_data = {}
    current_file = None
    lines_found = 0
    lines_hit = 0

    with open(lcov_file, 'r') as f:
        for line in f:
            line = line.strip()

            if line.startswith('SF:'):
                # Source file
                current_file = line[3:]
                lines_found = 0
                lines_hit = 0
            elif line.startswith('LF:'):
                # Lines found
                lines_found = int(line[3:])
            elif line.startswith('LH:'):
                # Lines hit
                lines_hit = int(line[3:])
            elif line.startswith('end_of_record'):
                if current_file:
                    coverage_data[current_file] = (lines_hit, lines_found)
                current_file = None

    return coverage_data


def calculate_percentage(hit: int, found: int) -> float:
    """Calculate coverage percentage."""
    if found == 0:
        return 100.0
    return (hit / found) * 100.0


def normalize_path(path: str) -> str:
    """Normalize file path for comparison."""
    # Remove leading paths and focus on project-relative paths
    if '/src/' in path:
        return 'src/' + path.split('/src/', 1)[1]
    elif '/include/' in path:
        return 'include/' + path.split('/include/', 1)[1]
    return path


def generate_report(base_coverage: Dict[str, Tuple[int, int]],
                   pr_coverage: Dict[str, Tuple[int, int]],
                   pr_number: int) -> str:
    """Generate a markdown report comparing coverage."""

    # Normalize paths for comparison
    base_normalized = {normalize_path(k): v for k, v in base_coverage.items()}
    pr_normalized = {normalize_path(k): v for k, v in pr_coverage.items()}

    # Find all files that appear in either coverage report
    all_files = set(base_normalized.keys()) | set(pr_normalized.keys())

    # Filter to only include source files (not tests or examples)
    source_files = {f for f in all_files
                   if f.startswith('src/') or f.startswith('include/')}

    # Calculate changes
    changes = []
    for file_path in sorted(source_files):
        base_hit, base_found = base_normalized.get(file_path, (0, 0))
        pr_hit, pr_found = pr_normalized.get(file_path, (0, 0))

        base_pct = calculate_percentage(base_hit, base_found)
        pr_pct = calculate_percentage(pr_hit, pr_found)

        diff = pr_pct - base_pct

        # Only include files with changes or new/removed files
        if abs(diff) > 0.01 or base_found == 0 or pr_found == 0:
            changes.append({
                'file': file_path,
                'base_hit': base_hit,
                'base_found': base_found,
                'base_pct': base_pct,
                'pr_hit': pr_hit,
                'pr_found': pr_found,
                'pr_pct': pr_pct,
                'diff': diff
            })

    # Calculate overall coverage
    total_base_hit = sum(v[0] for v in base_normalized.values())
    total_base_found = sum(v[1] for v in base_normalized.values())
    total_pr_hit = sum(v[0] for v in pr_normalized.values())
    total_pr_found = sum(v[1] for v in pr_normalized.values())

    total_base_pct = calculate_percentage(total_base_hit, total_base_found)
    total_pr_pct = calculate_percentage(total_pr_hit, total_pr_found)
    total_diff = total_pr_pct - total_base_pct

    # Generate markdown report
    report = [
        "## Coverage Report",
        "",
        f"**Overall Coverage**: {total_pr_pct:.2f}% ({total_diff:+.2f}%)",
        "",
        f"- **Base**: {total_base_pct:.2f}% ({total_base_hit}/{total_base_found} lines)",
        f"- **PR**: {total_pr_pct:.2f}% ({total_pr_hit}/{total_pr_found} lines)",
        "",
    ]

    if not changes:
        report.extend([
            "✅ No coverage changes detected for source files.",
            "",
            "<sub>Coverage analysis complete. All source files maintain their coverage levels.</sub>"
        ])
    else:
        # Categorize changes
        improved = [c for c in changes if c['diff'] > 0.01]
        decreased = [c for c in changes if c['diff'] < -0.01]
        new_files = [c for c in changes if c['base_found'] == 0 and c['pr_found'] > 0]
        removed_files = [c for c in changes if c['base_found'] > 0 and c['pr_found'] == 0]

        if improved:
            report.extend([
                "### ✅ Coverage Improved",
                "",
                "| File | Base | PR | Change |",
                "|------|------|----|----|"
            ])
            for change in sorted(improved, key=lambda x: x['diff'], reverse=True):
                report.append(
                    f"| `{change['file']}` | {change['base_pct']:.2f}% | "
                    f"{change['pr_pct']:.2f}% | "
                    f"<span style='color:green'>+{change['diff']:.2f}%</span> |"
                )
            report.append("")

        if decreased:
            report.extend([
                "### ⚠️ Coverage Decreased",
                "",
                "| File | Base | PR | Change |",
                "|------|------|----|----|"
            ])
            for change in sorted(decreased, key=lambda x: x['diff']):
                report.append(
                    f"| `{change['file']}` | {change['base_pct']:.2f}% | "
                    f"{change['pr_pct']:.2f}% | "
                    f"<span style='color:red'>{change['diff']:.2f}%</span> |"
                )
            report.append("")

        if new_files:
            report.extend([
                "### 🆕 New Files",
                "",
                "| File | Coverage |",
                "|------|----------|"
            ])
            for change in sorted(new_files, key=lambda x: x['file']):
                report.append(
                    f"| `{change['file']}` | {change['pr_pct']:.2f}% "
                    f"({change['pr_hit']}/{change['pr_found']} lines) |"
                )
            report.append("")

        if removed_files:
            report.extend([
                "### 🗑️ Removed Files",
                "",
                "| File | Previous Coverage |",
                "|------|-------------------|"
            ])
            for change in sorted(removed_files, key=lambda x: x['file']):
                report.append(
                    f"| `{change['file']}` | {change['base_pct']:.2f}% |"
                )
            report.append("")

        report.extend([
            "<sub>Coverage report automatically generated for each commit. "
            "This comment will be updated as new commits are pushed.</sub>"
        ])

    return "\n".join(report)


def main():
    parser = argparse.ArgumentParser(
        description='Compare lcov coverage reports and generate markdown report'
    )
    parser.add_argument('base_coverage', type=Path,
                       help='Base branch coverage info file (lcov format)')
    parser.add_argument('pr_coverage', type=Path,
                       help='PR coverage info file (lcov format)')
    parser.add_argument('--output', type=Path, default=Path('coverage-report.md'),
                       help='Output markdown file')
    parser.add_argument('--pr-number', type=int, required=True,
                       help='Pull request number')

    args = parser.parse_args()

    # Validate input files
    if not args.base_coverage.exists():
        print(f"Error: Base coverage file not found: {args.base_coverage}",
              file=sys.stderr)
        sys.exit(1)

    if not args.pr_coverage.exists():
        print(f"Error: PR coverage file not found: {args.pr_coverage}",
              file=sys.stderr)
        sys.exit(1)

    # Parse coverage data
    print(f"Parsing base coverage from {args.base_coverage}...")
    base_coverage = parse_lcov_file(args.base_coverage)

    print(f"Parsing PR coverage from {args.pr_coverage}...")
    pr_coverage = parse_lcov_file(args.pr_coverage)

    # Generate report
    print(f"Generating coverage report...")
    report = generate_report(base_coverage, pr_coverage, args.pr_number)

    # Write report
    args.output.write_text(report)
    print(f"Coverage report written to {args.output}")

    # Also print to stdout for debugging
    print("\n" + "="*80)
    print(report)
    print("="*80)


if __name__ == '__main__':
    main()
