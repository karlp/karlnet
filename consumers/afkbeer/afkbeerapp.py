#!/usr/bin/env python3
import logging
import os
from gi.repository import Gtk
from gi.repository import GLib
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

GLib.threads_init()

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
	        if (kp['node'] == config['node']):
			log.info("Got a relevant node message")
			l1 = self.builder.get_object("l_value1")
			l1.set_text(str(kp["sensors"][config["sensor1"]]["value"]))
			l2 = self.builder.get_object("l_value2")
			l2.set_text(str(kp["sensors"][config["sensor2"]]["value"]))
					

	def quit(self, *args):
        	Gtk.main_quit()


if __name__ == '__main__':
	ui = MyWindow()
	Gtk.main()
