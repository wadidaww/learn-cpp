person = person_pb2.Person()
person.ParseFromString(serialized_data)

print(f"Name: {person.name}")
print(f"ID: {person.id}")
for email in person.email:
    print(f"Email: {email}")