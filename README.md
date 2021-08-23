# cpp-json

A basic JSON parser in modern C++ using only the standard library.

### Dependencies and build

Just a modern Clang like Clang 12 on Fedora 34.

```
$ make
$ ./main '{"a": [1, 2, null, { "c": 129 }]}'
{
  "a": [
    1.000000,
    2.000000,
    null,
    {
      "c": 129.000000
    }
  ]
}
```

### Nice error handling


```bash
$ ./main "$(cat ./test/cofax-bad.json)"
Unexpected token ',', type 'Syntax', index
Failed to parse at line 5, column 18
  "servlet-class": ,"org.cofax.cds.CDSServlet",
                   ^
```

### API

Just two main functions.

#### std::tuple<json::JSONValue, std::string> parse(std::string)

This turns a string into a `json::JSONValue`. If it fails, the second
element in the return tuple contains an error string.

#### std::string deparse(json::JSONValue)

This turns a `json::JSONValue` back into a JSON string.
