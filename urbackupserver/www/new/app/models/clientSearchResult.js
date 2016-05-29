class ClientSearchResult {
    constructor(request, clients, clientCount) {
        this.request = request;
        this.clients = clients;
        this.clientCount = clientCount;
        this.generatePages(request, clients);
    }
    generatePages(request, clients) {
        var pageNumber = 1;
        this.pages = [];
        while (this.clientCount > ((pageNumber - 1) * request.pageSize)) {
            this.pages.push(pageNumber);
            pageNumber++;
        }
    }
}
exports.ClientSearchResult = ClientSearchResult;
//# sourceMappingURL=clientSearchResult.js.map