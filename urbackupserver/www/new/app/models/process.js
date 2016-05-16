System.register([], function(exports_1, context_1) {
    "use strict";
    var __moduleName = context_1 && context_1.id;
    var Process;
    return {
        setters:[],
        execute: function() {
            Process = (function () {
                function Process(action, percentage) {
                    this.action = action;
                    this.percentage = percentage;
                }
                return Process;
            }());
            exports_1("Process", Process);
        }
    }
});
//# sourceMappingURL=process.js.map