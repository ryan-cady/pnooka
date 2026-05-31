var Clay = require('pebble-clay');
var clayConfig = require('./config');

var clay = new Clay(clayConfig);

Pebble.addEventListener('ready', function() {
    var stored = localStorage.getItem('clay-settings');
    if (!stored) { return; }
    try {
        Pebble.sendAppMessage(
            Clay.prepareSettingsForAppMessage(JSON.parse(stored)),
            function() { console.log('Sent saved settings to watch'); },
            function(e) { console.log('Failed to send settings: ' + JSON.stringify(e)); }
        );
    } catch (e) {
        console.error('Settings send error: ' + e);
    }
});
