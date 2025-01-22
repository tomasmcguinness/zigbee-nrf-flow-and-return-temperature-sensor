# Zigbee F.A.R.T. Sensor (Flow and Return Temperature)

A Zigbee Sensor designed to monitor flow and return temperatures. The sensor provides two temperature sensor probes in a single unit. It can be used to monitor a boiler's flow and return temperatures or the different between the tails of a radiator.

Battery powered, this sensor should run for over a year on a coin battery or 10 years on a 2000mAh battery.

## Compiling 

This code has been developed using NCS (nRF Connect Software), with pin outs and battery reading chosen for the Seeed XIAO nrf52840. It should run on any nRF52840 with some tweaking. 

To compile, run the following command in an `nRF Connect` terminal. The pristine build (-p) flag isn't always necessary.

```
west build -b xiao_ble -p
```

## Flashing

I flash my XIAO Boards using a J-Link programmer, but if you don't have one, you should be able to deploy the firmware using the Adafruit UF2 Bootloader.

### UF2

* I should point out that I have had no success with this method yet! *

First, connect the board via USB to your computer. Next, double click the Reset button. A new USB drive should appear.

The UF2 firmware should be located here:

```
zigbee-nrf-flow-and-return-temperature-sensor\build\zigbee-nrf-flow-and-return-temperature-sensor\zephyr\zephyr.uf2
```

Just copy and paste this file onto the USB drive and the application *should* start. 

### J-Link

If you have a J-Link programmer, you can connect the board and run this command.

```
west flash
```

## GPIOs

There are three GPIOs required. One supplies power to the voltage divider. The other two are ADC pins, which read the temperature probes.



## Zigbee ZCL

In order to maintain compatability, this sensor uses standard ZCL clusters.

There are three Endpoints 1, 5 & 7

Endpoint 1 contains the Basic cluster, the Identify Cluster and the Power Configuration Cluster.
Endpoint 5 contains a Temperature Measurement Cluster
Endpoint 7 contains a Temperature Measurement Cluster

## Battery Life

I've invested a lot of time to try and ensure long battery life. I've written a few blog posts detailing the journey, which are up on my blog. You can start here:

https://tomasmcguinness.com/2024/12/19/reducing-power-consumption-in-my-nrf52840-zigbee-sensor-with-the-help-of-a-nordic-semiconductors-power-profiler-ii-kit/

# Setup

Once powered up, you can add the device to your Zigbee network as you would any other.

# Next Steps

* [ ] Use reset button to reverse probes i.e. swap readings
* [ ] Ensure reliability
* [ ] Ensure battery life
