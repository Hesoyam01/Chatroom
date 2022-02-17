#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <MQTTAsync.h>


// broker.hivemq.com is a public broker hence we use an unique identifier for topic level 0 to avoid random messages.
#define ADDRESS				"tcp://broker.hivemq.com:1883"
#define UID_ROOM			"pnRX2da8n5"
#define DEFAULT_ROOM		"General"
#define USERNAME_MAX_LENGTH 20
#define KEEP_ALIVE_INTERVAL 20
#define CLEAN_SESSION		0
#define RETAINED			0
#define QOS					2
#define TIMEOUT				10000L


typedef struct {
	char *clientId;
} Context;

typedef struct {
	char *active;
	char *roomName;
} Topic;

void responseOnFailure(void *context, MQTTAsync_failureData *failureData) {
	if(failureData->code == MQTTASYNC_SUCCESS)
		return;

	printf("Send error code: %d.\n", failureData->code);
	if (failureData->message) {
		printf("Send error message: %s.\n", failureData->message);
	}
}

int messageArrived(Context *context, char *topicName, int topicLength, MQTTAsync_message *message)
{
	char *msg = (char*) message->payload;
	char *clientId = context->clientId;

	// TODO: Change to dynamic header 
	char userHeader[30];
	sprintf(userHeader, "[%s]: ", clientId);

	int isFromOurselfs = strncmp(userHeader, msg, strlen(userHeader)) == 0;
	if(isFromOurselfs) {
		printf("\r\033[A"); // Go to the beginning of the line and one up to overwrite stdin
		printf("\x1b[31;1m"); // Red color
	} 

	printf("%s\n", msg);

	// Reset color
	printf("\x1b[31;0m");

	MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topicName);
	return true;
}

// TODO: Limit topic length
// Set topic level 1 with a unique room identifier as topic level 0
char *setTopic(char *room) {
	static char topic[64];
	strcpy(topic, UID_ROOM);
	strcat(topic, "/");
	strcat(topic, room);

	return topic;
}

// Init chatroom
void initRoom(MQTTAsync client, MQTTAsync_responseOptions *response, Topic *topic) {
	char *initTopic = setTopic(DEFAULT_ROOM);

	if (MQTTAsync_subscribe(client, initTopic, QOS, response) != MQTTASYNC_SUCCESS) {
		printf("Couldn't connect to the server.\n");
	}
	else {
		topic->active = initTopic;
		topic->roomName = DEFAULT_ROOM;

		printf("Welcome in %s. Type in !help to see available commands.\n", DEFAULT_ROOM);
	};
}

// Switch room and update current topic level
void switchRoom(MQTTAsync client, MQTTAsync_responseOptions *response, Topic *topic, char *room) {
	char *targetTopic = setTopic(room);

	// Unsubscribe from the current room
	if (MQTTAsync_unsubscribe(client, topic->active, response) != MQTTASYNC_SUCCESS) {
		printf("An error occured while leaving %s.\n", topic->roomName);
	}
	else {
		printf("You left %s.\n", topic->roomName);
		topic->active = NULL;
		topic->roomName = "";

		//Subscribte to the new room
		if (MQTTAsync_subscribe(client, targetTopic, QOS, response) != MQTTASYNC_SUCCESS) {
			printf("Couldn't join %s.\n", room);
		}
		else {
			topic->active = targetTopic;
			topic->roomName = room;

			printf("Welcome in %s.\n", room);
		};
	};
}

void chatCommands(MQTTAsync client, MQTTAsync_responseOptions *response, Topic *topic, bool *close, char *input) {
	// Split the input string into two possible substrings.
	// First substring is a possible command, the second substring are possible parameters depending on the command.
	char delimiter = ' ';
	char *p;
	if ((p = strchr(input, delimiter))) {
		*p = '\0';
	}
	else {
		p = NULL;
	}

	// Check available commands
	if (strcmp(input, "!logout") == 0) {
		*close = true;
	}
	else if (strcmp(input, "!help") == 0) {
		printf("Following commands are available:\n!help: Shows available commands.\n!logout: Disconnects you from the server.\n!switch ROOMNAME: Switches to the given chatroom.\n");
	}
	else if (strcmp(input, "!switch") == 0) {
		if (p) {
			switchRoom(client, response, topic, p + 1);
		}
		else {
			printf("No roomname given.\n");
		}
	}
	else {
		printf("Command doesn't exist.\n");
	}
}

int main(int argc, char* argv[])
{
	//Initialize MQTT connection
	MQTTAsync client;
	MQTTAsync_connectOptions connectionOptions = MQTTAsync_connectOptions_initializer;
	MQTTAsync_responseOptions responseOptions = MQTTAsync_responseOptions_initializer;
	MQTTAsync_disconnectOptions disconnectOptions = MQTTAsync_disconnectOptions_initializer;
	MQTTAsync_message pubMessage = MQTTAsync_message_initializer;

	Context context;
	Topic topic;

	int returnCode;
	bool closeCommand = false;
	char clientId[USERNAME_MAX_LENGTH + 1];

	// MQTT Connection parameters
	connectionOptions.keepAliveInterval = KEEP_ALIVE_INTERVAL;
	connectionOptions.cleansession = CLEAN_SESSION;
	responseOptions.onFailure = responseOnFailure;

	// TODO: limit input for username
	// Choose Username/Set client id
	printf("Please choose a username:\n");
	fgets(clientId, USERNAME_MAX_LENGTH, stdin);
	clientId[strlen(clientId) - 1] = '\0';
	context.clientId = clientId;

	// Setup client and connect to Broker
	MQTTAsync_create(&client, ADDRESS, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	MQTTAsync_setCallbacks(client, &context, NULL, messageArrived, NULL);

	if ((returnCode = MQTTAsync_connect(client, &connectionOptions)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to connect, return code %d.\n", returnCode);
		exit(-1);
	}
	while (MQTTAsync_isConnected(client) == false) {}

	// Subscribe to default topic
	topic.active = NULL;
	topic.roomName = "";
	initRoom(client, &responseOptions, &topic);

	// Inputhandler
	char userInput[256];
	while (closeCommand == false) 
	{
		char payload[256];
		for(int i = 0; i < 256; i++)
		{
			payload[i] = '\0';
			userInput[i] = '\0';
		}

		strcat(payload, "[");
		strcat(payload, clientId);
		strcat(payload, "]: ");

		// Read user input
		fgets(userInput, sizeof(userInput), stdin);
		// Remove new line of user input.
		userInput[strlen(userInput) - 1] = '\0';

		if (userInput[0] == '!') {
			chatCommands(client, &responseOptions, &topic, &closeCommand, userInput);
		}
		else {
			// TODO: Remove name tag when long messages get splitted.
			strcat(payload, userInput);
			pubMessage.payload = payload;
			pubMessage.payloadlen = strlen(payload) + 1;
			pubMessage.qos = QOS;
			pubMessage.retained = RETAINED;

			MQTTAsync_sendMessage(client, topic.active, &pubMessage, &responseOptions);
		}
	}

	MQTTAsync_disconnect(client, &disconnectOptions);
	MQTTAsync_destroy(&client);
	printf("Disconnected.\n");
	return 0;
}