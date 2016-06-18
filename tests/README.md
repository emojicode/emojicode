# Tests

You can run these tests **from the root** of this repository:

```
make && make install && make tests
```

You might need to prepend `sudo` to `make install`.

(Warning: The tests contain a lot of nonsense. 😜)

## Structure

- `compilation`: Contains different compilation problems (from very simple to
  advanced) and expected output.
- `s`: Contains tests to test the s package.
