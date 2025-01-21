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

## Zigbee ZCL


## Battery Life

I've invested a lot of time to try and ensure long battery life. I've written a few blog posts detailing the journey.



# Setup

Once powered up, you can add the device to your Zigbee network as you would any other.

# Next Steps

* [ ] Use reset button to reverse probes i.e. swap readings
* [ ] Ensure reliability
* [ ] Ensure battery life