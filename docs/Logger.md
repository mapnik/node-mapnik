## Logger

#### Constructor

- No constructor - Severity level is only available via `mapnik.Logger` static instance

#### Methods

- `getSeverity()` : Returns integer which represents severity level
- `setSeverity(severity)` : Accepts level of severity as a mapnik constant
```
var logging = getSeverity();
```

```
mapnik.Logger.setSeverity(mapnik.Logger.NONE);
```

#### Available severity levels
- mapnik.Logger.DEBUG -> 0
- mapnik.Logger.WARN -> 1
- mapnik.Logger.ERROR (default) -> 2
- mapnik.Logger.NONE -> 3

