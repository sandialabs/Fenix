$(function () {
    var window_location = window.location.pathname.split('/');
    var current_page = window_location.pop();
    var current_version = window_location.pop();
    var base_path = window_location.join('/');
    $.get(base_path + '/version_selector.html', function (data) {
        // Inject version selector HTML into the page
        $('#projectnumber').html(data);

        // Event listener to handle version selection
        document.getElementById('versionSelector').addEventListener('change', function () {
            var selectedVersion = this.value;
            window.location.href = base_path + '/' + selectedVersion + '/' + current_page + window.location.hash;
        });

        // Set the selected option based on the current version
        $('#versionSelector').val(current_version);
    });
});
