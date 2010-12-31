/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <iostream>

#include "base/json.h"

int main(int argc, char** argv)
{
    JsonObject object;

    object.Set("favorite-drink", "Pan-Galactic Gargle Blaster");
    object.Set("favorite-number", 42);
    JsonArray* array = new JsonArray;
    object.Set("friends", array);
    array->Add(new JsonString("Ford Prefect"));
    array->Add(new JsonString("Trillian"));
    JsonObject* o1 = new JsonObject;
    o1->Set("name", "Marvin");
    o1->Set("manufacturer", "Sirius Cybernetics Corporation");
    array->Add(o1);

    JsonObject* nestedObject = new JsonObject;
    nestedObject->Set("true", true);
    nestedObject->Set("false", false);
    object.Set("tautologies", nestedObject);

    object.Set("What do we use to represent an empty set in JSON?", new JsonArray());
    object.Set("What does an empty object look like?", new JsonObject());

    std::string buffer;
    object.Serialize(buffer, true);
    std::cout << buffer;

    return 0;
}
