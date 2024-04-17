//template <typename T>
//void (*DDDSerialprint)(T);
//
//// Serialprint2A(x,y) yield()
//// Serialprint(x) yield()
//// Serialprintln2A(x,y) yield()
//// Serialprintln(x) yield()
//// ForceSerialprint2A(x,y) yield()
//// ForceSerialprint(x) yield()
//// ForceSerialprintln2A(x,y) yield()
//// ForceSerialprintln(x) yield()
//
//
//template <typename T>
//void EmptySerialprint(T x) {yield();}
//
//template <typename T>
//void ReadSerialprint(T x) {Serial.print(x);}
//
//
//void InitSerialPrint(bool isRS232)
//{
//	if (isRS232)
//	{
//		DDDSerialprint<String> = &EmptySerialprint<String>;
//	}
//	else
//	{
//		DDDSerialprint<String> = &ReadSerialprint<String>;
//	}
//}
//
//// #define Serialprint2A(x,y) Serial.print(x,y)
//// #define Serialprint(x) Serial.print(x)
//// #define Serialprintln2A(x,y) Serial.println(x,y)
//// #define Serialprintln(x) Serial.println(x)
//// #define ForceSerialprint2A(x,y) Serial.print(x,y)
//// #define ForceSerialprint(x) Serial.print(x)
//// #define ForceSerialprintln2A(x,y) Serial.println(x,y)
//// #define ForceSerialprintln(x) Serial.println(x)
