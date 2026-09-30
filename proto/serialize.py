import person_pb2

person = person_pb2.Person()
person.name = "Alice"
person.id = 123
person.email.append("alice@example.com")

serialized_data = person.SerializeToString()