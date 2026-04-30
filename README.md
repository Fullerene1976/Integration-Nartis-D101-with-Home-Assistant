<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<style type="text/css">
<!--
.style1 {font-family: Arial, Helvetica, sans-serif}
.style2 {font-family: Arial, Helvetica, sans-serif; font-weight: bold; }
-->
</style>
<body>
<p class="style1"><strong>Introduction</strong><br />
  This  solution is intended for owners of Nartis-I100 split electric meters, when the  meter comes with a Nartis-D101 remote display and requires the transmission of  readings to the Home Assistant.</p>
<p align="center" class="style1"><img src="resources/nartis_i100.jpg" alt="Nartis_I100" width="300" height="300" /><br />
</p>
<p class="style1">There are  several other options for receiving readings from Nartis and then transmitting  them to a smart home: via an optoport or an RF-433 wireless USB module.  However, these solutions have their limitations. So connecting via an optoport  requires laying a cable to an electric meter, which is not always possible, and  integration via an expensive wireless USB module uses a slow and not very  stable SPODES protocol.<br />
  </p>
<p class="style1">The  Nartis-D101 remote display interacts with the electric meter wirelessly at a  frequency of 433MHz using the relatively fast Wireless ModBUS protocol, while  the transmitted data is encrypted using the AES-128 GCM method and cannot be  decoded without the appropriate keys. </p>
<p class="style1"><br />
  When  analyzing the device itself, it was found that the HT6027 MCU, on the basis of  which the device is implemented, actively exchanges with external FLASH memory  based on EN25S80 during operation. So, eight minutes after connecting the power  supply via the USB connector, the Nartis-D101 reads the readings from the  electric meter and two minutes later writes them to FLASH memory via the SPI  bus. Then this operation is repeated every hour in automatic mode.</p>
<p class="style1"><br />
  The ESP32  module was selected to read the SPI bus, but during testing it turned out that  when using conventional commands such as digitalRead(PIN_MOSI) or return  GPIO.in&amp;MOSI_MASK, system performance is insufficient, since the exchange  between the Nartis-D101 components is performed at a fairly high frequency of  10 MHz.</p>
<p class="style1"><br />
  The ESP32  has a special I2S+DMA reading mode, which allows you to record pin states  directly into RAM with minimal CPU usage, while the module's performance is  sufficient for successful operation, as demonstrated by the <a href="https://github.com/lmcapacho/ESP32_LogicAnalyzer">ESP32_LogicAnalyzer  project from lmcapacho</a>.</p>
<p class="style1"><strong>Equipment:</strong><br />
  1. Remote  display Nartis-D101<br />
  2. 30-pin  ESP32 module based on ESP-32-D0WD-V3 chip (revision 3.1)<br />
  3. Wires<br />
  4. Soldering iron</p>
<table width="50%" border="0" align="center">
  <tr>
    <td><div align="center"><img src="resources/nartis_d101.jpg" alt="Nartis_D101" width="150" height="200" align="middle" /></div></td>
    <td><div align="center"><img src="resources/esp32_board.jpg" alt="ESP32_board" width="100" height="204" align="middle" /></div></td>
    <td><div align="center"><img src="resources/mgtf_wire.jpg" alt="wire" width="180" height="200" align="middle" /></div></td>
    <td><div align="center"><img src="resources/soldering_iron.jpg" alt="soldering_iron" width="210" height="200" align="middle" /></div></td>
  </tr>
</table>
<p class="style2"><span data-src-align="6:10">Сonnection</span> <span data-src-align="0:5">diagram</span>:</p>
<table width="50%" border="0" align="center">
  <tr>
    <td><div align="center"><img src="resources/en25s80.jpg" alt="EN25S80" width="300" height="200" /></div></td>
    <td><div align="center"><img src="resources/esp32_pinout.jpg" alt="ESP32_pinout" width="400" height="200" /></div></td>
  </tr>
</table>
<p>&nbsp;</p>
<table width="23%" border="0" align="center">
  <tr>
    <th width="8%" class="style1" scope="col">#</th>
    <th width="31%" class="style1" scope="col">ESP32</th>
    <th width="61%" class="style1" scope="col">Nartis-D101</th>
  </tr>
  <tr>
    <td class="style1"><div align="center">1</div></td>
    <td class="style1"><div align="center">D23</div></td>
    <td class="style1"><div align="center">DI   EN26S80 (5 pin)</div></td>
  </tr>
  <tr>
    <td class="style1"><div align="center">2</div></td>
    <td class="style1"><div align="center">D18</div></td>
    <td class="style1"><div align="center">CLK   EN26S80 (6 pin)</div></td>
  </tr>
  <tr>
    <td class="style1"><div align="center">3</div></td>
    <td class="style1"><div align="center">VIN</div></td>
    <td class="style1"><div align="center">+5V  USB</div></td>
  </tr>
  <tr>
    <td class="style1"><div align="center">4</div></td>
    <td class="style1"><div align="center">GND</div></td>
    <td class="style1"><div align="center">GND</div></td>
  </tr>
</table>
<p class="style2"><span data-src-align="5:11">Connection</span> <span data-src-align="0:4">photo</span>:</p>
<p align="center" class="style1"><img src="resources/pcb.jpg" alt="pcb" width="500" height="650" /></p>
<p class="style1"><span data-src-align="0:3">As</span> it <span data-src-align="4:9">turned</span> out<span data-src-align="13:1">,</span> the <span data-src-align="15:5">ESP32</span> <span data-src-align="31:10">fits</span> <span data-src-align="21:9">perfectly</span> <span data-src-align="42:1">in</span> the <span data-src-align="44:9">dimensions</span> of the <span data-src-align="54:11">battery</span> <span data-src-align="66:6">compartment</span><span data-src-align="72:1">,</span> <span data-src-align="74:7">so</span> <span data-src-align="82:3">it</span> <span data-src-align="86:5">can</span> be <span data-src-align="92:10">placed</span> <span data-src-align="103:5">directly</span> <span data-src-align="109:6">inside</span> the <span data-src-align="116:9">Nartis-D101</span><span data-src-align="126:7"></span> by <span data-src-align="135:9">printing</span> the <span data-src-align="145:6">case</span> <span data-src-align="152:2">on</span> a <span data-src-align="155:2">3D</span> <span data-src-align="158:8">printer</span>:</p>
<table width="30%" border="0" align="center">
  <tr>
    <th scope="col"><img src="resources/case1.jpg" alt="case_1" width="180" height="200" /></th>
    <th scope="col"><img src="resources/case2.jpg" alt="case_2" width="200" height="200" /></th>
  </tr>
</table>
<p class="style1">The <span data-src-align="18:5">ESP32</span> <span data-src-align="0:5">can</span> <span data-src-align="24:1">also</span> be <span data-src-align="6:11">positioned</span> <span data-src-align="26:7">outside</span><span data-src-align="33:1">,</span> and <span data-src-align="39:5">it</span> is <span data-src-align="45:6">convenient</span> to <span data-src-align="52:12">use</span> a <span data-src-align="65:1">4</span><span data-src-align="66:1">-</span><span data-src-align="67:3">pin</span> <span data-src-align="71:5">audio</span> <span data-src-align="77:6">jack</span> for this:</p>
<table width="50%" border="0" align="center">
  <tr>
    <th scope="col"><img src="resources/audio_4_f.jpg" alt="audio_f" width="160" height="150" /></th>
    <th scope="col"><img src="resources/audio_4_m1.jpg" alt="audio_m1" width="200" height="150" /></th>
    <th scope="col"><img src="resources/audio_4_m2.jpg" alt="audio_m2" width="200" height="150" /></th>
  </tr>
</table>
<p class="style1"><span data-src-align="0:5">Important</span><span data-src-align="5:1">:</span> <span data-src-align="7:3">when</span> <span data-src-align="11:5">soldering</span> <span data-src-align="17:10">connections</span>, <span data-src-align="28:10">try</span> to <span data-src-align="39:7">make</span> the <span data-src-align="47:7">wires</span> <span data-src-align="55:11">as</span> <span data-src-align="67:9">short</span> as possible<span data-src-align="76:1">!</span> </p>
<p class="style2"><span data-src-align="79:23">Software</span><span data-src-align="102:1">:</span> </p>
<p class="style1"><span data-src-align="107:1">1</span><span data-src-align="108:1">.</span> <span data-src-align="110:7">Arduino</span> <span data-src-align="118:3">IDE</span> <span data-src-align="122:2">v2</span><span data-src-align="124:1">.</span><span data-src-align="125:1">3</span><span data-src-align="126:1">.</span><span data-src-align="127:1">8</span> </p>
<p class="style1"><span data-src-align="130:1">2</span><span data-src-align="131:1">.</span> <span data-src-align="133:5">ESP32</span> <span data-src-align="139:2">by</span> <span data-src-align="142:17">Espressif</span> Systems <span data-src-align="160:2">v3</span><span data-src-align="162:1">.</span><span data-src-align="163:1">3</span><span data-src-align="164:1">.</span><span data-src-align="165:1">8</span> </p>
<p class="style1"><span data-src-align="168:1">3</span><span data-src-align="169:1">.</span> <span data-src-align="171:12">PubSubClient</span> <span data-src-align="184:2">by</span> <span data-src-align="187:4">Nick</span> <span data-src-align="192:2">O</span><span data-src-align="194:5">'Leary</span> <span data-src-align="200:2">v2</span><span data-src-align="202:1">.</span><span data-src-align="201:1">8</span> </p>
<p class="style1"><span data-src-align="204:5">Before</span> <span data-src-align="210:11">compiling</span> the <span data-src-align="222:7">project</span>, <span data-src-align="230:8">select</span> the <span data-src-align="245:5">ESP32</span> <span data-src-align="251:3">Dev</span> <span data-src-align="255:6">Module</span> <span data-src-align="239:5">board</span><span data-src-align="261:1">.</span> <span data-src-align="273:5">Board</span> <span data-src-align="263:9">parameters</span> <span data-src-align="279:3">for</span> this <span data-src-align="283:8">scetch</span>:</p>
<p align="center" class="style1"><img src="resources/arduino_ide.jpg" alt="arduino_parameters" width="450" height="400" /></p>
<p class="style1"><span data-src-align="0:5">Important</span><span data-src-align="5:1">:</span> it is <span data-src-align="7:10">necessary</span> to <span data-src-align="18:10">provide</span> <span data-src-align="29:11">sufficient</span> <span data-src-align="49:1">+</span><span data-src-align="50:2">5V</span> <span data-src-align="41:7">power</span> <span data-src-align="53:2">on</span> the <span data-src-align="56:3">USB</span> <span data-src-align="60:5">port</span><span data-src-align="65:1">,</span> <span data-src-align="67:3">as</span> <span data-src-align="77:4">it</span> <span data-src-align="82:12">simultaneously</span> <span data-src-align="95:8">powers</span> the <span data-src-align="104:8">external</span> <span data-src-align="113:7">display</span> <span data-src-align="121:1">and</span> the <span data-src-align="130:5">ESP32</span> <span data-src-align="123:6">module</span><span data-src-align="135:1">!</span> </p>
<p class="style1"><span data-src-align="138:3">For</span> <span data-src-align="142:7">easy</span> <span data-src-align="150:10">integration</span> <span data-src-align="161:1">with</span> the <span data-src-align="163:4">Home</span> <span data-src-align="168:9">Assistant</span>, the <span data-src-align="199:13">Autodiscovery</span> <span data-src-align="191:7">function</span> is <span data-src-align="178:12">used</span><span data-src-align="212:1">,</span> <span data-src-align="214:3">while</span> the <span data-src-align="223:10">device</span> is <span data-src-align="234:13">automatically</span> <span data-src-align="248:14">registered</span> with <span data-src-align="265:4">MQTT</span><span data-src-align="269:1">.</span> </p>
<p class="style1">It is <span data-src-align="285:5">also</span> <span data-src-align="271:13">recommended</span> to <span data-src-align="291:12">use</span> a <span data-src-align="304:9">timestamp</span> <span data-src-align="314:3">to</span> <span data-src-align="318:8">monitor</span> the <span data-src-align="345:6">module</span>'s <span data-src-align="327:17">health</span><span data-src-align="351:1">.</span> <span data-src-align="353:6">Just</span> <span data-src-align="360:8">add</span> the <span data-src-align="369:9">following</span> <span data-src-align="379:6">lines</span> <span data-src-align="386:1">to</span> the <span data-src-align="393:13">configuration</span><span data-src-align="406:1">.</span><span data-src-align="407:4">yaml</span> <span data-src-align="388:4">file</span>:</p>
<p>template:<br />
  &nbsp; &nbsp;- sensor:<br />
  &nbsp; &nbsp; &nbsp; &nbsp;-  name: &quot;Timestamp&quot;<br />
  &nbsp; &nbsp; &nbsp; &nbsp;  &nbsp;unique_id: NartisTimestamp<br />
  &nbsp; &nbsp; &nbsp; &nbsp;  &nbsp;state: &quot;{{ states.sensor.nartis_d101_total.last_updated }}&quot;<br />
  &nbsp; &nbsp; &nbsp; &nbsp;  &nbsp;device_class: timestamp</p>
<p class="style1">An <span data-src-align="0:6">example</span> of a <span data-src-align="7:8">card</span> <span data-src-align="16:1">in</span> the <span data-src-align="18:4">Home</span> <span data-src-align="23:9">Assistant</span>:</p>
<p class="style1">&nbsp;</p>
<p align="center" class="style1"><img src="resources/home_assistant_card.jpg" alt="ha_card" width="400" height="400" /></p>
<p class="style1"><span data-src-align="0:5">Before</span> <span data-src-align="6:8">filling</span> in the <span data-src-align="15:6">sketch</span>, <span data-src-align="22:7">set</span> the <span data-src-align="30:9">parameters</span> of <span data-src-align="40:5">your</span> <span data-src-align="46:4">WiFi</span> <span data-src-align="51:4">network</span> <span data-src-align="56:1">and</span> <span data-src-align="58:4">MQTT</span> <span data-src-align="63:7">broker</span> <span data-src-align="71:1">in</span> the nartis_d101<span data-src-align="90:1">.</span><span data-src-align="91:3">ino</span> <span data-src-align="73:5">file</span>:</p>
<p align="center" class="style1"><img src="resources/my_own_setings.jpg" alt="wifi&amp;mqtt_settings" width="485" height="170" /></p>
<p class="style1">Good <span data-src-align="0:7">luck</span> <span data-src-align="8:1">in</span> <span data-src-align="10:10">implementing</span> <span data-src-align="21:5">your</span> <span data-src-align="27:4">ideas</span><span data-src-align="31:1">!</span></p>
</body>
</html>
