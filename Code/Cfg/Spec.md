# Config Query Sub-Language

Supported by the API and used to query for nodes, attributes, and values within a cfg document in a concise way.

## Specification

Specified in the [Extended Backus–Naur Form](https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_Form) below.

```
AnyChar = ? Any UTF-8 character ?

DigitWithoutZero = "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
Digit = "0" | DigitWithoutZero
PositiveNumber = DigitWithoutZero, {Digit}

SpecialChar = "!" | '' | "#" | "$" | "%" | "&" | "'" |
              "(" | ")" | "*" | "+" | "," | "-" | "." |
              "/" | ":" | ";" | "<" | "=" | ">" | "?" |
              "@" | "[" | "\" | "]" | "^" | "_" | "`" |
              "{" | "|" | "}" | "~"

IdentChar = ? AnyChar without SpecialChar and Digit ?

Identifier = ( IdentChar | "_" ), { IdentChar | "_" | Digit };

NodeSpec = Identifier, [ "[", PositiveNumber, "]" ]
AttributeSpec = "@", Identifier
ValueSpec = "#", PositiveNumber

Query = NodeSpec, [{ "/", NodeSpec }], ([ AttributeSpec ] | [ ValueSpec ]);
```

## Examples

| Query String | Equivalent Query | Result |
| ------------ | ---------------- | ------ |
| `Foo/Bar` | `Foo[0]/Bar[0]#0` | Look for the first node called "Foo", find its first "Bar" child and fetch the first value of it. |
| `Foo@Bar` | `Foo[0]@Bar` | Find the first node called "Foo" and look for its attribute called "Bar" and return its value. |
