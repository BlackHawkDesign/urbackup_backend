System.register([], function(exports_1, context_1) {
    "use strict";
    var __moduleName = context_1 && context_1.id;
    var Client;
    return {
        setters:[],
        execute: function() {
            Client = (function () {
                function Client(name) {
                    this.name = name;
                    this.selected = false;
                }
                return Client;
            }());
            exports_1("Client", Client);
        }
    }
});
//# sourceMappingURL=client.js.map