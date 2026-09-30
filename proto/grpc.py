import grpc
import person_pb2
import person_pb2_grpc

# Create gRPC channel and stub
channel = grpc.insecure_channel('localhost:50051')
stub = person_pb2_grpc.PersonServiceStub(channel)

# Create a request message
request = person_pb2.PersonRequest()
request.name = "Alice"
request.id = 123
request.email = "alice@example.com"

# Call RPC method
response = stub.GetPerson(request)

print(f"Received: {response}")