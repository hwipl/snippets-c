/* mqtt subscribe client using libmosquitto */

/* mosquitto, printf */
#include <mosquitto.h>
#include <stdio.h>

/* broker address and port, topic */
const char *broker = "localhost";
int port = 1883;
const char *topic = "hello/world";

/* connect callback */
void on_connect(struct mosquitto *client, void *obj, int rc) {
	/* check connection result */
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "error connecting to broker: %s\n",
			mosquitto_connack_string(rc));
		return;
	}
	printf("client connected.\n");

	/* subscribe */
	int msg_id;
	const char *sub = topic;
	int qos = 0;
	rc = mosquitto_subscribe(client, &msg_id, sub, qos);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "error subscribing to topic: %s\n",
			mosquitto_strerror(rc));
	}
	printf("subscribing to topic %s with message id %d\n", topic, msg_id);
}

/* disconnect callback */
void on_disconnect(struct mosquitto *client, void *obj, int rc) {
	if (rc) {
		fprintf(stderr, "unexpected disconnect\n");
	}
	printf("client disconnected.\n");
}

/* subscribe callback */
void on_subscribe(struct mosquitto *client, void *obj, int msg_id, int
		  qos_count, const int *granted_qos) {
	printf("subscribed with message id %d\n", msg_id);
}

/* message callback */
void on_message(struct mosquitto *client, void *obj, const struct
		mosquitto_message *msg) {
	if (!msg->payloadlen){
		/* ignore empty messages */
		return;
	}
	printf("%s: %s\n", msg->topic, msg->payload);
}

int main() {
	int rc = 0;

	/* check topic */
	rc = mosquitto_sub_topic_check(topic);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "error in topic: %s\n",
			mosquitto_strerror(rc));
		return rc;
	}

	/* init */
	mosquitto_lib_init();
	const char *client_id = NULL;
	bool clean_session = true;
	void *obj = NULL;
	struct mosquitto *client = mosquitto_new(client_id, clean_session, obj);
	if (!client) {
		rc = -1;
		fprintf(stderr, "error creating client\n");
		goto lib_cleanup;
	}

	/* set callbacks */
	mosquitto_connect_callback_set(client, on_connect);
	mosquitto_disconnect_callback_set(client, on_disconnect);
	mosquitto_subscribe_callback_set(client, on_subscribe);
	mosquitto_message_callback_set(client, on_message);

	/* connect */
	int keepalive = 30;
	rc = mosquitto_connect(client, broker, port, keepalive);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "error connecting to broker: %s\n",
			mosquitto_strerror(rc));
		goto cleanup;
	}

	/* loop */
	int timeout = -1;
	int max_packets = 1; /* unused */
	rc = mosquitto_loop_forever(client, timeout, max_packets);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "error in main loop: %s\n",
			mosquitto_strerror(rc));
	}

	/* cleanup */
cleanup:
	mosquitto_destroy(client);
lib_cleanup:
	mosquitto_lib_cleanup();
	return rc;
}
