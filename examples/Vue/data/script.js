var app;

function init() {
    app = new Vue({
        el: '#app',
        components: {
            ColorPicker: window.VueColorPicker
        },
        data: {
            msg: 'Welcome to Your YoYo Vue.js App',
            color: {
                hue: 0,
                red: 0,
                green: 0,
                blue: 0,
            },
        },
        methods: {
            onInput: function(hue) {
                hue =  Math.floor(hue);
                this.color.hue = hue;
                setHSL(hue/360.0, 1.0, 0.5);
            },
            onChange: function(hue) {
                axios.post('/yoyo/colour', this.color);
            },
            onSelect: function(hue) {
            },
        }
    });
    getRGB();
}

//Adapted from: https://gist.github.com/mjackson/5311256
function setHSL(h, s, l) {
    if (s == 0) {
        app.color.red = app.color.green = app.color.blue = l; // achromatic
    }
    else {
        var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        var p = 2 * l - q;

        r = Math.floor(hue2rgb(p, q, h + 1/3) * 255);
        g = Math.floor(hue2rgb(p, q, h) * 255);
        b = Math.floor(hue2rgb(p, q, h - 1/3) * 255);

        app.color.red = r;
        app.color.green = g;
        app.color.blue = b;
    }
}

function getRGB() {
    axios.get('/yoyo/colour')
        .then((response) => {
            hue = Math.floor(rgb2hue(response.data.red, response.data.green, response.data.blue));
            app.color.hue = hue;
            setHSL(hue/360.0, 1.0, 0.5);
        });
}

function hue2rgb(p, q, t) {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1/6) return p + (q - p) * 6 * t;
    if (t < 1/2) return q;
    if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
    return p;
}

//From: https://stackoverflow.com/questions/23090019/fastest-formula-to-get-hue-from-rgb
function rgb2hue(r, g, b) {
    hue = 0;

    r = r/255;
    g = g/255;
    b = b/255;

    min = Math.min(r, g, b);
    max = Math.max(r, g, b);

    if(r > g && r > b)      hue = (g-b)/(max-min);
    else if(g > r && g > b) hue = 2.0 + (b-r)/(max-min);
    else if(b > r && b > g) hue = 4.0 + (r-g)/(max-min);
    hue *= 60;

    return(hue);
}