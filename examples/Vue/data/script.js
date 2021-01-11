var ColorPicker = window.VueColorPicker;

var app = new Vue({
    el: '#app',
    components: {
        ColorPicker: ColorPicker
    },
    data: {
        msg: 'Welcome to Your YoYo Vue.js App',
        colour: {
            red: 0,
            green: 0,
            blue: 0,
        },
    },
    methods: {
        onInput: function(hue) {
            setRGB(hue/360.0, 1.0, 0.5);
        },
        onChange: function(hue) {
            axios.post('/yoyo/colour', this.colour);
        },
        onSelect: function(hue) {
        },
    }
});
setRGB(0, 1.0, 0.5);

//Adapted from: https://gist.github.com/mjackson/5311256
function setRGB(h, s, l) {
    if (s == 0) {
        app.colour.red = app.colour.green = app.colour.blue = l; // achromatic
    }
    else {
        var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        var p = 2 * l - q;

        app.colour.red = Math.floor(hue2rgb(p, q, h + 1/3) * 255);
        app.colour.green = Math.floor(hue2rgb(p, q, h) * 255);
        app.colour.blue = Math.floor(hue2rgb(p, q, h - 1/3) * 255);
    }
}

function hue2rgb(p, q, t) {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1/6) return p + (q - p) * 6 * t;
    if (t < 1/2) return q;
    if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
    return p;
}