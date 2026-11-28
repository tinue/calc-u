# pytest driver for the PC-1500, PC-1500A and PC-1600

The user guide is [`docs/testing/README.md`](../../docs/testing/README.md): setup, a test project of
your own, worked examples, paths, the reference, how it works.

| File | Content |
|------|---------|
| `calcu_pytest.py` | The pytest plugin: the fixtures `make_machine` and `machine_class`, the path names of `pytest.ini` |
| `machine.py` | Class `Machine`: models, slots, printer, media, bus ROMs, `listing()`, `card()` |
| `cardimage.py` | Parser for saved module files and the RAM disk on them (no machine needed) |
| `test_driver.py`, `test_plugin.py` | Tests of the driver, the parser and the plugin |
| `../../../../library/harness/python/calcu_machine.py` | Class `Session`, the product-neutral part |
| `../../../../library/harness/python/report.py` | The run's Markdown report (`calcu_report`) |

The driver's own tests (CTest `CalcU1600MachineDriver`, Longer), from `products/calc-u-1600/`:

```sh
uv run --no-project --with pytest --with pytest-xdist pytest -n auto tools/machine
```
