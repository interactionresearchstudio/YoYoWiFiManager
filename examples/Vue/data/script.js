var ColorPicker = window.VueColorPicker;

var app = new Vue({
    el: '#app',
    components: {
        ColorPicker: ColorPicker
    },
    data: {
        msg: 'Welcome to Your Vue.js App',
        color: {
            hue: 50,
            saturation: 100,
            luminosity: 50,
            alpha: 1,
        },
    },
    methods: {
        onInput: function(hue) {
            this.color.hue = hue;
        },
        onChange: function(hue) {
            this.color.hue = hue;
            
            axios.post('/yoyo/colour', this.color);
        },
        onSelect: function(hue) {
            this.color.hue = hue;
        },
    }
});