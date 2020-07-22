:title "Lecture notes in sexual HCI" 1: capacitive touch sensing prosthetic penis
:author RT

**Content Warning**: bodies and electronics discussed in a sexual context, described at a concrete level with some images but no graphic depictions of sex or genitalia

As a budding HCI researcher, I have been interested in the intersection between sexuality and human-computer interaction (or HMI) for the last couple of years. I begun this project in 2018 by exploring different sensors that could be used for a touch-sensitive prosthetic penis, the main target audience at the time being trans men.

~~

**Content Warning**: bodies and electronics discussed in a sexual context, described at a concrete level without any graphic images

As a budding HCI researcher, I have been interested in the intersection between sexuality and human-computer interaction (or HMI) for the last couple of years. I begun this project in 2018 by exploring different sensors that could be used for a touch-sensitive prosthetic penis, the main target audience at the time being trans men. 

I should mention that I am not trans myself, however I have had the opportunity to get feedback from a trans male tester during development, and the software and physical device has been optimized for the context of a trans guy topping a cis guy. Regardless, your experience may vary should you choose to build this project yourself.

Unfortunately even though I now have a working prototype of the prosthetic, it would be very difficult to acquire the necessary resources for a publication-ready study. I also feel that the HCI research community is not quite ready for a publication that deals with sexuality at this level of concreteness.  This is in spite of the fact that there is some positive activity and previous work, for example the CHI Queer Special Interest Group and previous papers in sexual HCI. Much of this activity has in my opinion been marked by a theoretical emphasis. In this project, I have tried to deal with bodies and pleasures directly, taking more of an engineering approach. What follows will be more or less just a technical post.

## TECHNICAL DETAILS
The touch sensing uses three electrodes made from aluminum foil. One electrode is placed on the skin (for grounding purposes only), and two sensing electrodes (the _base_ and _tip_ electrodes) are placed at opposite ends on the prosthetic. This design is inspired by capacitive touch sliders such as those found in some kitchen appliances, adapted for use on the human body by adding the grounding electrode and using more sophisticated algorithms for continuous calibration. Continuous calibration is very important, because the electrical environment around the prosthetic is constantly changing due to differing amounts and types of lubricant, contact with the surrounding skin and the skin of other people that may interact with the prosthetic.

This design allows us to detect proximity of touch (pressure) to either sensing electrode individually (the _pressure speed_ signal from the tip electrode is used), and also calculate the approximate location of the touch point between the two electrodes. From the position, we can further calculate the speed of movement with direction (the _stroking speed_ signal). The control module (the brains of the device) contains an ESP32 breakout board with built-in charging circuit and a 600 mAh LiPo battery for powering the device. Power should be sufficient for up to two hours of operation. The ESP32's built-in touch sensor module is used, so no additional hardware is required.

The control module translates the two signals (tip pressure speed and stroking speed) into a power level for the _actuator_. Only one actuator is supported right now, the Lovense Domi 2 wand, although the software should work for other Lovense devices as well with only minor modifications.. The control module automatically connects to the wand using Bluetooth Low Energy communications.

The signals are translated according to a curve chosen from a number of presets. Anecdotally, this type of sensing is sufficient to feel the effects of stroking and pressure on the prosthetic, and the stimulation helps to achieve orgasm during penetration and improves the connection between the user and the prosthetic. There is some latency inherent in the system, mainly due to signal filtering, BLE transmission and the time it takes for the actuator motor to spin up. I have tried to minimize the latency as much as possible, but it is still in the order of a hundred milliseconds. I believe this mainly comes from the motor, so using different actuators may improve latency. I have also implemented some prediction to try to compensate for the latency, but it is currently disabled by default. You can use the "cth" command with a lower threshold to try it out.

### Control module interface
To enable the actuator, you need to connect to the control module and enable vibration. You may use the nRF Connect app for iOS and Android phones to connect, and type text commands into the control module's primitive command line interface. There is no GUI app at this point, although I might consider developing one if there is enough interest. There is something exciting about having a CLI into your prosthetic penis, as absurd as it sounds.
I have provided some hastily written instructions for the CLI at the end of this post together with the source code.

The control module masquerades as a Bluetooth-enabled phone with the appropriate name and icon. However, sophisticated scanners could detect this device as the MAC address does not match with a known phone manufacturer, but should instead register as an ESP32. You can change the device name in the source code if needed.

## HOW TO BUILD IT
### Parts: 
- WEMOS Lite ESP32 Breakout board (https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20200627085927&SearchText=wemos+lite)
- 600 mAh LiPo battery (https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20200627090059&SearchText=600mah+battery)
- 55x35x15mm project case (https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20200627091101&SearchText=55x35x15mm+project+case)
- Suitable penis extender sleeve (https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20200627091302&SearchText=Soft+Silicone+Penis+Extender+Reusable)
- Dupont wire (for connecting the electrodes) (https://www.aliexpress.com/wholesale?catId=0&SearchText=dupont+wire)
- Aluminum foil for the electrodes
- Plastic wrap
- Spare Parts Deuce Harness (has a bigger pouch for storing the control module)
- Lovense Domi 2 Wand

### Instructions
**YOU NEED TO HAVE EXPERIENCE WITH ELECTRONICS TO BE ABLE TO BUILD THIS PROJECT AND HANDLE LiPo BATTERIES SAFELY. THE MATERIAL PRESENTED (DOCUMENTATION AND SOURCE CODE) IS SCIENTIFIC AND TECHNICAL IN NATURE, AND DESCRIBES AN UNFINISHED PROTOTYPE WHICH HAS NOT BEEN TESTED AND IS NOT MEANT FOR CONSUMER USE. CAREFULLY READ THE FOLLOWING DISCLAIMER RELATING TO THE SOFTWARE AND THE ASSOCIATED DOCUMENTATION IN THE SOURCE CODE REPOSITORY AND THIS PAGE. BY USING THE SOFTWARE OR DOCUMENTATION, YOU AGREE TO THE FOLLOWING AS A CONDITION OF YOUR LICENSE:**

**THE SOFTWARE AND DOCUMENTATION IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE AND DOCUMENTATION OR THE USE OR OTHER DEALINGS IN THE SOFTWARE AND DOCUMENTATION. **

1. Solder pin headers to ESP32 Breakout.
2. Attach the LiPo battery to the other side of the ESP32 Breakout (side opposite to the LiPo connector) with tape. Solder LiPo wires to JST connector and push the connector into the socket on the breakout board, or alternatively remove the LiPo socket on the breakout and solder the battery wires directly onto the pads. Ensure effective insulation between battery wires.
3. Cut a hole in the project case for the Micro USB connector
4. Place ESP32 Breakout with attached battery into the project case. Tape the ESP32 board into place.
5. Connect ground electrode to GND pin on ESP32 breakout
6. Connect tip electrode to pin 4 on the ESP32 breakout
7. Connect base electrode to pin 2 on the ESP32 breakout
8. Connect ESP32 breakout to PC with Micro USB cable and upload the software with the Arduino IDE (link at end of post)
9. Place the contraption into a sock (only during active use, not while charging) and lead the wires out

#### Electrodes
The electrodes are made by cutting the Dupont wire on one end, and folding the exposed wire inside a plate made out of aluminum foil. Use multiple layers of aluminum foil for more durability. The ground electrode is placed on the body in contact with the skin (behind the harness). Cut the base and tip electrodes so that a gap is left when they are wrapped around the wand. The wires are run through this gap. The Dupont connectors on the wires may come loose with multiple insertions. Consider replacing the wires often, taping the connectors together, and using socket connectors instead of pin connectors for the first connection (only the socket connector loosens with use). Otherwise you may be faced with an unplanned debugging session if wires come loose during play.

#### Images

![Photo of the prototype device without sleeve](https://realtimewuff.com/files/img/device.jpg)


#### Finalizing
The wand part is wrapped in plastic wrap, and a condom is unfolded on top. Leave a gap in the plastic wrap at the tip for charging the wand, but fold the plastic wrap over the charging port when in use (to avoid lube from the condom getting into the charging port).
Finally, power on the wand and unfold the penis extender sleeve over the condom. Put a condom on the extender sleeve for good measure. The device is now ready for use.

You should experiment with several different sleeves to find one that is the appropriate thickness. I have found that oil-based lube (coconut oil) works better than water-based. This may be due to the electrical properties of water-based lube. Be aware that oil-based lube may degrade latex condoms.

#### First boot
Turn on the wand, reboot the control module with the RST button and it should automatically connect to the wand via bluetooth. On a successful connection, the wand should vibrate briefly. Connect to the control module via the nRF Connect app (using the device name and pin), and issue the "toggle" command in TEXT mode to the last characteristic in the list. This will enable vibration. To lighten the load on the BLE interface, consider disconnecting from the control module after issuing your commands.

## FURTHER DEVELOPMENT
The software could be further refined, especially the prediction part. I am considering adding support for multiple actuators at the same time, and also support for other actuator models, for example suction-based stimulators. This way, the stimulation could be moved between two different actuators, perhaps one internal and one external. There is some research that suggests this could result in a phantom sensation somewhere in between the two actuators, adding a sense of position to the sensation in addition to just varying the intensity of stimulation. Finally, a third sensing channel could be added for touch-sensitive testicles, although embedding the electrode would be more challenging than with the current sleeve design. 

## RESOURCES
Source code and instructions: 

You can contact me at rt@realtimewuff.com if you have any questions on how to build the device or want to give feedback. Right now, I am putting this project on the backburner, but I may be motivated to make further improvements if I get enough feedback. As of now, I am uncertain as to how many people are out there who would actually attempt to build their own device.