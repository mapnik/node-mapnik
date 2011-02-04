
/* obj pool to manage concurrency */


create = function(size){
    return {
        max: size || 5,
        pools: {},
        acquire: function(id, options, callback) {
            if (!this.pools[id]) {
                var that = this;
                this.pools[id] = require('generic-pool').Pool({
                    name: id,
                    create: options.create,
                    destroy: options.destroy,
                    max: that.max,
                    idleTimeoutMillis: 5000,
                    log: false
                    //reapIntervalMillis
                    //priorityRange
                });
            }
            this.pools[id].acquire(callback,options.priority);
        },
        release: function(id, obj) {
            this.pools[id] && this.pools[id].release(obj);
        }
    }
};

exports.create = create;

