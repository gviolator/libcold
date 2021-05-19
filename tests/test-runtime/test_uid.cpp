#include "pch.h"
#include <cold/utils/uid.h>

//#include "serialization/jsonserialization.h"
//#include "serialization/msgpackserialization.h"
//#include "io/bytesbufferwriter.h"

using namespace cold;
using namespace testing;



TEST(Common_Uid, ConstructedByDefaultIsEmpty)
{
	const Uid emptyUid;

	ASSERT_FALSE(emptyUid);
}


TEST(Common_Uid, Generate)
{
	const auto uid1 = Uid::generate();

	ASSERT_TRUE(uid1);

	const auto uid2 = Uid::generate();

	ASSERT_TRUE(uid2);

	ASSERT_THAT(uid1, Not(Eq(uid2)));
}


TEST(Common_Uid, Stringize) {
	const auto uid1 = Uid::generate();
	const auto str = toString(uid1);
	ASSERT_FALSE(str.empty());

	const auto wstr = toWString(uid1);
	ASSERT_FALSE(wstr.empty());
	ASSERT_THAT(str, Eq(strings::toUtf8(wstr)));
}


TEST(Common_Uid, Parse)
{
	const auto originalUid = Uid::generate();

	const auto str = toString(originalUid);

	const auto parsedUid = Uid::parse(str);

	ASSERT_TRUE(parsedUid);

	ASSERT_THAT(*parsedUid, Eq(originalUid));
}


TEST(Common_Uid, FailParseInvalidUid)
{
	const char* invalidUid = "non guid";

	const auto parsedUid = Uid::parse(invalidUid);

	ASSERT_FALSE(parsedUid);
}

#if 0
TEST(Common_Uid, JsonSerialization)
{
	using namespace serialization;

	const auto uid = Uid::generate();

	auto r = meta::represent(uid);

	auto json = json::jsonSerializeToString(uid, json::JsonIndentation::Pretty);

	const auto deserializedUid = json::jsonDeserialize<Uid>(json);

	ASSERT_THAT(deserializedUid, Eq(uid));
}
#endif

//TEST(Common_Uid, MsgpackSerialization)
//{
//	using namespace serialization;
//
//	const auto uid = Uid::generate();
//
//	BytesBuffer buffer;
//
//	Msgpack::msgpackSerialize(uid, buffer);
//
//	const auto deserializedUid = Msgpack::msgpackDeserialize<Uid>(buffer.toReadOnly());
//
//	ASSERT_THAT(deserializedUid, Eq(uid));
//}


