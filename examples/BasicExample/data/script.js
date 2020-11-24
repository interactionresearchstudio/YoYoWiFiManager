function init() {
    $('#config').hide();
    $('#nextstep').hide();
    $('#alert-text').hide();

    $('#networks-list-select').attr('disabled', true);
    $('#local_pass').attr('disabled', true);

    $.getJSON('/credentials', function (json) {
        $('#config').show();
        configure(json);
    });
}

function configure(json) {
    console.log(json);

    $('#save-button').click(onSaveButtonClicked);
    $('#local_pass').keypress(onKeyPressed);

    //$('#local-scad-code').text(formatScadCode(json.local_mac));
    $('[id="local-scad-code"]').text(formatScadCode(json.local_mac));
    if (json.remote_mac != "") {
        $('#remote-scad-code').text(formatScadCode(json.remote_mac));
        $('#remote-scad-code-input').hide();
    } else {
        $('#remote-scad-text').hide();
    }

    configureDisplay(json.local_paired_status);

    populateNetworksList(json.local_ssid);
    //TODO: use the password length to display
}

function configureDisplay(local_paired_status) {
    switch(local_paired_status) {
        case 'remoteSetup':
            console.log("remote");
            //Show local wifi form, local and remote IDs
            $('#remoteWifiForm').show();
            $('#remoteMacForm').show();
            break;
        case 'localSetup':
            console.log("local");
            //Show local wifi form and remote wifi form
            $('#remoteWifiForm').show();
            $('#remoteMacForm').hide();
            break;
        case 'pairedSetup':
            //just show local wifi details
            console.log("paired");
            $('#remoteWifiForm').show();
            $('#remoteMacForm').hide();
            break;
    }
}

function formatScadCode(code) {
    return(code.slice(0, 4) + ' ' + code.slice(4));
}

function onKeyPressed(event) {
    if (event.keyCode == 13) {
        onSaveButtonClicked(event);
    }
}

function populateNetworksList(selectedNetwork) {
    let networks = $('#networks-list-select');

    $.getJSON('/scan', function (json) {
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
        local_pass: $('#local_pass').val(),
        remote_mac: $('#remote-scad-code-input').val().replace(/\s/g,''),
        remote_ssid: $('#remote_ssid').val(),
        remote_pass: $('#remote_pass').val()
    };

    if ($('#remoteMacForm').is(":visible") && data.remote_mac == "") {
      $('#remote-scad-code-input').addClass('is-invalid');
      $('#alert-text').show();
      $('#alert-text').addClass('alert-danger');
      $('#alert-text').text('Please enter the required fields below.');
      $(window).scrollTop(0);
      return;
    }

    //NB dataType is 'text' otherwise json validation fails on Safari
    $.ajax({
        type: "POST",
        url: "/credentials",
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

            reboot(10000);
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

function reboot(t) {
    $.ajax({
        type: "POST",
        url: "/reboot",
        data: JSON.stringify({ delay: t }),
        dataType: 'text',
        contentType: 'application/json; charset=utf-8',
        cache: false,
        timeout: 15000,
        async: false,
        success: function(response, textStatus, jqXHR) {
            console.log(response);
        },
        error: function (jqXHR, textStatus, errorThrown) {
            console.log(jqXHR);
            console.log(textStatus);
            console.log(errorThrown);
        }
    });
}
