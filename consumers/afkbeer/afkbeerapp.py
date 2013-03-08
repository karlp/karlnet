#!/usr/bin/env python
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
	"sensors" : [
		{"sensor_num" : 2, "name" : "kettle probe" },
		{"sensor_num" : 0, "name" : "Ambient temp" }
	]
}

logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s",
#filename="/var/log/karlnet_pachube.log"
)
log = logging.getLogger("main")

class AlarmedSensor():
	"""
	Ideally, this would be a gtk widget, that could respond to signals directly and stuff
	"""
	fileuri = "file:///home/karlp/src/karlnet-git/consumers/pachube/phone_2.wav"
	playing = False
	enabled = False
	threshold = None
	def __init__(self, name):
		self.name = name
		self.ilog = logging.getLogger(self.__class__.__name__ + "_" + self.name)
		self.player = Gst.ElementFactory.make("playbin2", "player")
		fakesink = Gst.ElementFactory.make("fakesink", "fakesink")
		self.player.set_property("video-sink", fakesink)
		self.player.set_property("uri", self.fileuri)
		self.player.connect("about-to-finish", self.on_about_to_finish)

	def on_about_to_finish(self, player):
		"""need to just restart it again"""
		player.set_property("uri", self.fileuri)

	def handle_enabled(self, enabled):
		self.enabled = enabled.get_active()
		if not self.enabled:
			self.play(False)

	def handle_update_threshold(self, adjustment):
		self.threshold = adjustment.get_value()

	def play(self, on):
		self.ilog.info("playing sound: %s", on)
		# Don't try and restart if we're already playing
		#if self.playing and on:
		#	return
		if on and self.enabled:
			self.player.set_state(Gst.State.PLAYING)
			self.playing = True
		else:
			self.player.set_state(Gst.State.PAUSED)
			self.playing = False

	def update(self, currentvalue):
		if self.threshold is None:
			self.ilog.warn("Treshold can't be none...")
			return
		cooling = self.threshold < 50
		heating = not cooling
		self.ilog.debug("threshold is %d, mode: %s", self.threshold, "cooling" if cooling else "heating")
		if (cooling and currentvalue < self.threshold) or (heating and currentvalue > self.threshold):
			self.ilog.info("Ok, we're ready!")
			self.play(True)
		else:
			self.ilog.info("Turning sound off, we're no longer in range")
			self.play(False)


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

		s1name = config["sensors"][0]["name"]
		self.alarm1 = AlarmedSensor(s1name)
		self.builder.get_object("l_desc_sensor1").set_text(s1name)
		target1 = self.builder.get_object("adjustment1")
		target1.connect("value-changed", self.alarm1.handle_update_threshold)
		enabled1 = self.builder.get_object("chk_alarm_value1")
		enabled1.connect("toggled", self.alarm1.handle_enabled)

		s2name = config["sensors"][1]["name"]
		self.alarm2 = AlarmedSensor(s2name)
		self.builder.get_object("l_desc_sensor2").set_text(s2name)
		target2 = self.builder.get_object("adjustment2")
		target2.connect("value-changed", self.alarm2.handle_update_threshold)
		enabled2 = self.builder.get_object("chk_alarm_value2")
		enabled2.connect("toggled", self.alarm2.handle_enabled)
		

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


	def on_message(self, mosq, obj, msg):
		if debug:
			self.l_last_msg_topic.set_text(msg.topic)
			self.l_last_msg_payload.set_text(msg.payload)
		kp = jsonpickle.decode(msg.payload)
	        if (kp['node'] != config['node']):
			return

		log.info("Got a relevant node message")
		l1_val = kp["sensors"][config["sensors"][0]["sensor_num"]]["value"]
		l2_val = kp["sensors"][config["sensors"][1]["sensor_num"]]["value"]
		l1 = self.builder.get_object("l_value1")
		l1.set_text(str(l1_val))
		l2 = self.builder.get_object("l_value2")
		l2.set_text(str(l2_val))
		self.alarm1.update(l1_val)
		self.alarm2.update(l2_val)

	def quit(self, *args):
        	Gtk.main_quit()


if __name__ == '__main__':
	GObject.threads_init()
	Gst.init_check(sys.argv)
	GLib.threads_init()
	ui = MyWindow()
	Gtk.main()
