/* mqtt publish client using libmosquitto */

/* mosquitto, printf, strnlen */
#include <mosquitto.h>
#include <stdio.h>
#include <string.h>

/* maximum payload length */
#define MAX_PAYLOAD_LEN 256

/* broker address and port, topic, payload */
const char *broker = "localhost";
int port = 1883;
const char *topic = "hello/world";
const char *payload = "hi";

/* connect callback */
void on_connect(struct mosquitto *client, void *obj, int rc) {
	/* check connection result */
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "error connecting to broker: %s\n",
			mosquitto_connack_string(rc));
		return;
	}
	printf("client connected.\n");

	/* publish */
	int msg_id;
	int payload_len = strnlen(payload, MAX_PAYLOAD_LEN);
	int qos = 0;
	bool retain = false;
	rc = mosquitto_publish(client, &msg_id, topic, payload_len, payload,
			       qos, retain);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "error publishing: %s\n",
			mosquitto_strerror(rc));
	}
	printf("publishing message with id %d to topic %s: %s\n", msg_id, topic,
	       payload);
}

/* disconnect callback */
void on_disconnect(struct mosquitto *client, void *obj, int rc) {
	if (rc) {
		fprintf(stderr, "unexpected disconnect\n");
	}
	printf("client disconnected.\n");
}

/* publish callback */
void on_publish(struct mosquitto *client, void *obj, int msg_id) {
	fprintf(stdout, "published message with id %d\n", msg_id);

	/* disconnect */
	int rc = mosquitto_disconnect(client);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "error disconnecting from broker: %s\n",
			mosquitto_strerror(rc));
	}
}

int main() {
	int rc = 0;

	/* check topic */
	rc = mosquitto_pub_topic_check(topic);
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
	mosquitto_publish_callback_set(client, on_publish);

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
