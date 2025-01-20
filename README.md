## Building 

This code has been developed for the Seeed XIAO nrf52840, thought it should run on any nRF52840. 

To compile, run the following command. The pristine build (-p always) isn't always necessary.

```
west build -p always -b xiao_ble/nrf52840
```

Once successfully compiled, flash the firmware onto the device using

```
west flash
```

## GPIOs

There are three GPIOs required. One supplies power to the voltage divider. The other two are ADC pins, which read the temperature probes.

## DK
nrf52840dk/nrf52840