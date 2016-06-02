"use strict";
var clientSearchRequest_1 = require('./clientSearchRequest');
var ClientSearchResult = (function () {
    function ClientSearchResult(request, clients, clientCount) {
        this.request = request || new clientSearchRequest_1.ClientSearchRequest();
        this.clients = clients || [];
        this.clientCount = clientCount || 0;
        this.generatePages();
    }
    ClientSearchResult.prototype.generatePages = function () {
        var pageNumber = 1;
        this.pages = [];
        while (this.clientCount > ((pageNumber - 1) * this.request.pageSize)) {
            this.pages.push(pageNumber);
            pageNumber++;
        }
    };
    return ClientSearchResult;
}());
exports.ClientSearchResult = ClientSearchResult;
//# sourceMappingURL=clientSearchResult.js.map