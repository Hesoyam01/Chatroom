#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <MQTTAsync.h>

#define ADDRESS		"127.0.0.1:1883"
#define QOS			2
#define TIMEOUT     10000L

typedef struct {
	char *clientId;
} Context;

void responseOnFailure(void* context, MQTTAsync_failureData* failureData) {
	if(failureData->code == MQTTASYNC_SUCCESS)
		return;

	printf("Send error code: %d\n", failureData->code);
	if (failureData->message) {
		printf("Send error message: %s\n", failureData->message);
	}
}

int messageArrived(Context *context, char* topicName, int topicLength, MQTTAsync_message* message)
{
	char *msg = (char*) message->payload;
	char *clientId = context->clientId;

	char userHeader[30];
	sprintf(userHeader, "[%s]: ", clientId);

	int isFromOurselfs = strncmp(userHeader, msg, strlen(userHeader)) == 0;
	if(isFromOurselfs) {
		printf("\r\033[A"); // Go to the beginnging of the line and one up to overwrite stdin
		printf("\x1b[31;1m"); // Red color
	} 

	printf("%s\n", msg);

	// Reset color
	printf("\x1b[31;0m");

	MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topicName);
	return true;
}

int main(int argc, char* argv[]) 
{
	MQTTAsync client;
	MQTTAsync_connectOptions connectionOptions = MQTTAsync_connectOptions_initializer;
	MQTTAsync_responseOptions responseOptions = MQTTAsync_responseOptions_initializer;
	responseOptions.onFailure = responseOnFailure;
	MQTTAsync_disconnectOptions disconnectOptions = MQTTAsync_disconnectOptions_initializer;
	MQTTAsync_message pubMessage = MQTTAsync_message_initializer;
	MQTTAsync_token token = 0;

	int returnCode;
	bool closeCommand = false;

	// Choose Username
	printf("Choose username: ");
	char clientId[20];
	fgets(clientId, sizeof(clientId), stdin);
	clientId[strlen(clientId) - 1] = '\0';
	Context context;
	context.clientId = clientId;

	MQTTAsync_create(&client, ADDRESS, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);

	// Choose room number
	printf("Choose room number: ");
	int num;
	scanf("%d\n", &num);
	char topic[20];
	sprintf(topic, "chat/%d", num);

	// MQTT Connection parameters
	connectionOptions.keepAliveInterval = 20;
	connectionOptions.cleansession = 1;

	//Connect to Broker
	if ((returnCode = MQTTAsync_connect(client, &connectionOptions)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to connect, return code %d\n", returnCode);
		exit(-1);
	}
	while (MQTTAsync_isConnected(client) == false) {}
	printf("Connected to Chatroom: %s\n", topic);

	//Subscribe Topic
	MQTTAsync_setCallbacks(client, &context, NULL, messageArrived, NULL);
	returnCode = MQTTAsync_subscribe(client, topic, QOS, &responseOptions);

	printf("%d\n", returnCode);

	//Inputhandler
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

		fgets(userInput, sizeof(userInput), stdin); //read userInput

		if (strcmp(userInput, "::exit\n") == 0) {
			closeCommand = true;
		}
		else {
			strcat(payload, userInput);
			pubMessage.payload = payload;
			pubMessage.payloadlen = strlen(payload) + 1;
			pubMessage.qos = QOS;
			pubMessage.retained = 1;
			MQTTAsync_sendMessage(client, topic, &pubMessage, &responseOptions);
		}
	}

	MQTTAsync_disconnect(client, &disconnectOptions);
	MQTTAsync_destroy(&client);
	printf("Disconnected");
	return 0;
}