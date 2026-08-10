// Move operation badges to title line
document.addEventListener('DOMContentLoaded', function() {
    // Find all operation badges
    const badges = document.querySelectorAll('.operation-badge-inline');

    badges.forEach(function(badge) {
        // Get the parent paragraph
        const para = badge.parentElement;

        // Find the preceding h1 (the title)
        const section = para.closest('section');
        if (section) {
            const title = section.querySelector('h1');
            if (title) {
                // Move the badge into the h1
                title.appendChild(badge);
            }
        }
    });
});
