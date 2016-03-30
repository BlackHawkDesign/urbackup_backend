System.register([], function(exports_1, context_1) {
    "use strict";
    var __moduleName = context_1 && context_1.id;
    var ClientSearchResult;
    return {
        setters:[],
        execute: function() {
            ClientSearchResult = (function () {
                function ClientSearchResult(request, clients) {
                    this.request = request;
                    this.clients = clients;
                }
                return ClientSearchResult;
            }());
            exports_1("ClientSearchResult", ClientSearchResult);
        }
    }
});
//# sourceMappingURL=clientSearchResult.js.map