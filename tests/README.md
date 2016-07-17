# Tests

You can run these tests **from the root** of this repository once you have
installed Emojicode.

```
make tests
```

(Warning: The tests contain a lot of nonsense. 😜)

## Structure

- `compilation`: Contains different compilation problems (from very simple to
  advanced) and expected output.
- `s`: Contains tests to test the s package.
- `reject`: Contains invalid code or otherwise invalid operations that must be
  rejected by the compiler.
