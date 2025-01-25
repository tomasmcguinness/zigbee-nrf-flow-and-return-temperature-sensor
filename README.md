# Zigbee F.A.R.T. Sensor (Flow and Return Temperature) on nRF62840

A Zigbee Sensor designed to monitor flow and return temperatures. The sensor provides two temperature sensor probes in a single unit. It can be used to monitor a boiler's flow and return temperatures or the different between the tails of a radiator.

Battery powered, this sensor should run for over a year on a coin battery or 10 years on a 2000mAh battery.

## Compiling 

This code has been developed using NCS (nRF Connect Software), with pin outs and battery reading chosen for the Seeed XIAO nrf52840. It should run on any nRF52840 with some tweaking. 

To compile, run the following command in an `nRF Connect` terminal. The pristine build (-p) flag isn't always necessary.

```
west build -b xiao_ble -p
```

## Flashing

The XIAO board can be flashed using two methods. The first is done using the factory installed bootloader. The second is done using a J-Link programmer. I have a J-Link programmer, so that's the method I've been using.

### UF2

> [!CAUTION] 
> Whilst this method *should* work, I haven't have any success! I want to make this project accessible to those without a J-Link, so I will dedicate time to it.

First, connect the board via USB to your computer. Next, double click the Reset button. A new USB drive should appear.

The compilation process will generate a UF2 firmware file and it will be be located here:

```
zigbee-nrf-flow-and-return-temperature-sensor\build\zigbee-nrf-flow-and-return-temperature-sensor\zephyr\zephyr.uf2
```

Just copy and paste this file onto the USB drive and the application *should* start. 

### J-Link

If you have a J-Link programmer, connect the board and run this command.

```
west flash
```

## Pairing

Once you have flashed the nRF52840 and powered it up, it will enter pairing mode. You can then add it to your Zigbee network using your usual method.

# GPIOs

There several GPIOs configured. These have been picked for the Seeed XIAO nRF52840 board.

![image](https://github.com/user-attachments/assets/3c12b195-e805-4862-b072-d9325f9bd02d)

P0.03 is the ADC (AIN1) pin for Probe 1
P0.28 is the ADC (AIN5) pin for Probe 2 
P0.28 providers power for the voltage dividers connected to the NTC temperature probes

P1.13 is the reset pin. Apply high for 10 seconds to reset the Zigbee configuration.

# Zigbee ZCL

In order to maintain compatability, this sensor uses standard ZCL clusters rather.

There are three Endpoints 1, 5 & 7

Endpoint 1 contains the Basic cluster, the Identify Cluster and the Power Configuration Cluster.
Endpoint 5 contains a Temperature Measurement Cluster
Endpoint 7 contains a Temperature Measurement Cluster

# Home Assistant

I have tested my sensor on HomeAssistant using the ZHA integration and it displays pretty well. 

![image](https://github.com/user-attachments/assets/c9312264-5ab0-4d7b-bd74-c8124017b7d5)

Unfortunately, the probe names are terrible.

```
sensor.coldbear_fart_sensor_temperature
sensor.coldbear_fart_sensor_temperature_2
```

I have tried to make a ZHA "Quirk" to display the probes like this:

```
sensor.coldbear_fart_sensor_flow_temperature
sensor.coldbear_fart_sensor_return_temperature
```

but I haven't had any success yet making it work how I want. I have included my work in progress in the project.

# Battery Life

I've invested a lot of time to try and ensure long battery life and I think I've gotten it to over a year on a coin battery! Average consumption is around 16µA

![image](https://github.com/user-attachments/assets/b5269a33-37d2-41da-a05c-f229f9e24b03)

I've written quite a few blog posts detailing the journey, which are up on my blog. You can start here:

https://tomasmcguinness.com/2024/12/19/reducing-power-consumption-in-my-nrf52840-zigbee-sensor-with-the-help-of-a-nordic-semiconductors-power-profiler-ii-kit/

# Setup

Once powered up, you can add the device to your Zigbee network as you would any other Zigbee device.

# PCB

> [!CAUTION]
> If you choose to use version 0.1 of the PCB, the reset button will not work. I've outline the reason here https://tomasmcguinness.com/2025/01/24/f-a-r-t-sensor-pcb-mistake/

Using KiCAD, I have designed a PCB for this code. However, there are some flaws in it. I have included the first version for reference.

If you are interested in buying a PCB, do let me know. If there is enough interest, I can place a bulk order.

# Next Steps

* [ ] Fix reset button PCB traces
* [ ] Use reset button to reverse probes i.e. swap readings
* [ ] Ensure reliability
* [ ] Ensure battery life
