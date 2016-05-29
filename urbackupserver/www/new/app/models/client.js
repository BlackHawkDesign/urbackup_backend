class Client {
    constructor(name) {
        this.name = name;
        this.selected = false;
        this.showDetail = false;
        this.processes = [];
    }
    isCreatingBackups() {
        return this.processes.length > 0;
    }
    getTotalProcessPercentage() {
        var totalPercentange = 0;
        for (var i in this.processes) {
            totalPercentange += this.processes[i].percentage;
        }
        return this.processes.length === 0 ? totalPercentange : totalPercentange / this.processes.length;
    }
}
exports.Client = Client;
//# sourceMappingURL=client.js.map