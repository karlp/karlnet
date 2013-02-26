#!/usr/bin/env python3
import logging
import os, sys
from gi.repository import Gtk
from gi.repository import GLib
from gi.repository import Gst
from gi.repository import GObject
import jsonpickle
import mosquitto

APPNAME="AFK Beer Console"

debug = True
config = {
	"node" : 4369,
	"sensor1" : 0,
	"sensor2" : 2
}

logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s",
#filename="/var/log/karlnet_pachube.log"
)
log = logging.getLogger("main")


class MyWindow(Gtk.Window):

	def __init__(self):
		self.builder = Gtk.Builder()
		self.builder.add_from_file(os.path.join(os.getcwd(), 'afkbeer.ui'))
		self.window = self.builder.get_object('window1')
		self.window.set_title(APPNAME)
		self.window.connect('delete-event', self.quit)
		self.window.show()
		self.connect_button = self.builder.get_object("btn_connect")
		self.connect_button.connect('toggled', self.connect_toggle)
		self.entry_mqtt_host = self.builder.get_object("entry_mqtt_host")
		self.l_last_msg_topic = self.builder.get_object("l_last_msg_topic")
		self.l_last_msg_payload = self.builder.get_object("l_last_msg_payload")
		if not debug:
			self.l_last_msg_topic.set_visible(False)
			self.l_last_msg_payload.set_visible(False)
		self.player = Gst.ElementFactory.make("playbin2", "player")
		fakesink = Gst.ElementFactory.make("fakesink", "fakesink")
		self.player.set_property("video-sink", fakesink)
		self.player.set_property("uri", "file:///home/karlp/src/karlnet-git/consumers/pachube/phone_2.wav")
		

	def connect_toggle(self, button):
		hostfield = self.builder.get_object('entry_mqtt_host')
		if button.get_active():
			button.set_label("Connected")
			hostfield.set_editable(False)
			self.mqttc = mosquitto.Mosquitto("afkbeer", clean_session=True)
			self.mqttc.on_message = self.on_message
			host = self.entry_mqtt_host.get_text()
			self.mqttc.connect(self.entry_mqtt_host.get_text())
			self.mqttc.loop_start()
			self.mqttc.subscribe("karlnet/readings/%s/#" % (config["node"]), 0)
		else:
			self.mqttc.disconnect()
			button.set_label("Connect")
			hostfield.set_editable(True)

	def playsound(self, on):
		log.info("playing sound: %s", on)
		if on:
			self.player.set_state(Gst.State.PLAYING)
		else:
			self.player.set_state(Gst.State.PAUSED)

	def handle_potential_alarm(self, current, threshold, enabled):
		if not enabled:
			# FIXME - need to turn it off though if it was on because of this alarm
			log.info("Alarm not enabled for this target, ignoring")
			return
		log.info("current value: %d, target:%d, should we do something?", current, threshold)

		if threshold is not None:
			cooling = threshold < 50
			heating = not cooling
			log.debug("threshold is %d, mode: %s", threshold, "cooling" if cooling else "heating")
		# Assume cooling if the setTemp is under 50, assume heating if the set temp is over 50....
		if (cooling and current < threshold) or (heating and current > threshold):
			log.warn("ok, it's ready!")
			self.playsound(True)
		else:
			if (heating):
				log.info("Turning music off.... we're below the threshold")
			else:
				log.info("Turning music off.... we're above the threshold")
			self.playsound(False)


	def on_message(self, mosq, obj, msg):
		if debug:
			self.l_last_msg_topic.set_text(msg.topic)
			self.l_last_msg_payload.set_text(msg.payload)
		kp = jsonpickle.decode(msg.payload)
	        if (kp['node'] != config['node']):
			return

		log.info("Got a relevant node message")
		l1_val = kp["sensors"][config["sensor1"]]["value"]
		l2_val = kp["sensors"][config["sensor2"]]["value"]
		l1 = self.builder.get_object("l_value1")
		l1.set_text(str(l1_val))
		l2 = self.builder.get_object("l_value2")
		l2.set_text(str(l2_val))
		target1 = self.builder.get_object("adjustment1").get_value()
		enabled1 = self.builder.get_object("chk_alarm_value1").get_active()
		target2 = self.builder.get_object("adjustment2").get_value()
		enabled2 = self.builder.get_object("chk_alarm_value2").get_active()
		self.handle_potential_alarm(l1_val, target1, enabled1)
		self.handle_potential_alarm(l2_val, target2, enabled2)

	def quit(self, *args):
        	Gtk.main_quit()


if __name__ == '__main__':
	GObject.threads_init()
	Gst.init_check(sys.argv)
	GLib.threads_init()
	ui = MyWindow()
	Gtk.main()
