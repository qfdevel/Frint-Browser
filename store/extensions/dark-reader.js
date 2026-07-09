// Frint Browser - Dark Reader Extension
// Applies dark mode to all websites

(function() {
    'use strict';

    const DARK_MODE_CSS = `
        html {
            filter: invert(0.9) hue-rotate(180deg) !important;
        }
        img, video, canvas, svg, [style*="background-image"] {
            filter: invert(1) hue-rotate(180deg) !important;
        }
        body {
            background: #1a1a2e !important;
            color: #cdd6f4 !important;
        }
    `;

    class DarkReader {
        constructor() {
            this.enabled = true;
            this.styleElement = null;
        }

        enable() {
            if (this.styleElement) return;
            this.styleElement = document.createElement('style');
            this.styleElement.textContent = DARK_MODE_CSS;
            this.styleElement.id = 'frint-dark-reader';
            document.head.appendChild(this.styleElement);
            this.enabled = true;
        }

        disable() {
            const el = document.getElementById('frint-dark-reader');
            if (el) el.remove();
            this.styleElement = null;
            this.enabled = false;
        }

        toggle() {
            if (this.enabled) this.disable();
            else this.enable();
        }
    }

    const darkReader = new DarkReader();
    darkReader.enable();

    // Listen for toggle messages
    window.addEventListener('message', function(event) {
        if (event.data && event.data.type === 'dark-reader-toggle') {
            darkReader.toggle();
        }
    });
})();
