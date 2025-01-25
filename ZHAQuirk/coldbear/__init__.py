""" Cold Bear FART Sensor """

import logging

from zigpy.quirks.v2 import QuirkBuilder, CustomCluster, SensorStateClass
from zigpy.quirks.v2.homeassistant import UnitOfTemperature
from zigpy.zcl.clusters.general import AnalogInput, Basic, Identify, Ota
from zigpy.zcl.clusters.measurement import TemperatureMeasurement
from zigpy.zcl.foundation import BaseAttributeDefs, ZCLAttributeDef
import zigpy.types as t
from typing import Final

_LOGGER = logging.getLogger(__name__)

class Probe1TemperatureCluster(CustomCluster, TemperatureMeasurement):   
    """Probe"""

    def _update_attribute(self, attrid, value):
        super()._update_attribute(attrid, value)


class Probe2TemperatureCluster(CustomCluster, TemperatureMeasurement):
     """Probe"""

     def _update_attribute(self, attrid, value):
        super()._update_attribute(attrid, value)

(
    QuirkBuilder("coldbear", "FART Sensor")
    .replaces(Probe1TemperatureCluster, endpoint_id=5)
    .replaces(Probe2TemperatureCluster, endpoint_id=7)
    .sensor(
        Probe1TemperatureCluster.AttributeDefs.measured_value.name,
        Probe1TemperatureCluster.cluster_id,
        endpoint_id=5,
        state_class=SensorStateClass.MEASUREMENT,
        unit=UnitOfTemperature.CELSIUS,
        translation_key="flow_temperature",
        fallback_name="Flow Temperature",
    )
    .sensor(
        Probe2TemperatureCluster.AttributeDefs.measured_value.name,
        Probe2TemperatureCluster.cluster_id,
        endpoint_id=7,
        state_class=SensorStateClass.MEASUREMENT,
        unit=UnitOfTemperature.CELSIUS,
        translation_key="return_temperature",
        fallback_name="Return Temperature",
    )
    .add_to_registry()
)