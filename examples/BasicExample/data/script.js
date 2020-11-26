function init() {
    $('#config').hide();
    $('#nextstep').hide();
    $('#alert-text').hide();

    $('#networks-list-select').attr('disabled', true);
    $('#local_pass').attr('disabled', true);

    // $.getJSON('/credentials', function (json) {
    //     $('#config').show();
    //     configure(json);
    // });
    $('#config').show();

    $('#save-button').click(onSaveButtonClicked);
    $('#local_pass').keypress(onKeyPressed);

    populateNetworksList("");
}

function onKeyPressed(event) {
    if (event.keyCode == 13) {
        onSaveButtonClicked(event);
    }
}

function populateNetworksList(selectedNetwork) {
    let networks = $('#networks-list-select');

    $.getJSON('/yoyo/networks', function (json) {
        networks.empty();
        $.each(json, function (key, entry) {
            let network = $('<option></option>');

            network.attr('value', entry.SSID).text(entry.SSID);
            if(entry.SSID == selectedNetwork) network.attr('selected', true);

            networks.append(network);
        });

        $('#networks-list-select').attr('disabled', false);
        $('#local_pass').attr('disabled', false);
    });
}

function onSaveButtonClicked(event) {
    event.preventDefault();

    var data = {
        local_ssid: $('#networks-list-select').children("option:selected").val(),
        local_pass: $('#local_pass').val()
    };

    //NB dataType is 'text' otherwise json validation fails on Safari
    $.ajax({
        type: "POST",
        url: "/yoyo/settings",
        data: JSON.stringify(data),
        dataType: 'text',
        contentType: 'application/json; charset=utf-8',
        cache: false,
        timeout: 15000,
        async: false,
        success: function(response, textStatus, jqXHR) {
            console.log(response);
            $('#config').hide();
            $('#alert-text').show();
            $('#alert-text').removeClass('alert-danger');
            $('#alert-text').addClass('alert-success');
            $('#alert-text').text('Saved');
            $('#nextstep').show();

            //reboot(10000);
        },
        error: function (jqXHR, textStatus, errorThrown) {
            console.log(jqXHR);
            console.log(textStatus);
            console.log(errorThrown);
            $('#alert-text').show();
            $('#alert-text').addClass('alert-danger');
            $('#alert-text').text('Couldn\'t Save');
        }
    });
}