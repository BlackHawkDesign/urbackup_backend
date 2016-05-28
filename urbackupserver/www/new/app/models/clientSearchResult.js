"use strict";
var ClientSearchResult = (function () {
    function ClientSearchResult(request, clients, clientCount) {
        this.request = request;
        this.clients = clients;
        this.clientCount = clientCount;
        this.generatePages(request, clients);
    }
    ClientSearchResult.prototype.generatePages = function (request, clients) {
        var pageNumber = 1;
        this.pages = [];
        while (this.clientCount > ((pageNumber - 1) * request.pageSize)) {
            this.pages.push(pageNumber);
            pageNumber++;
        }
    };
    return ClientSearchResult;
}());
exports.ClientSearchResult = ClientSearchResult;
//# sourceMappingURL=clientSearchResult.js.map