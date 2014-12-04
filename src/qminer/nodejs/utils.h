#ifndef QMINER_NODEJS_UTILS
#define QMINER_NODEJS_UTILS

#include <node.h>
#include "base.h"
#include "qminer.h"

#define JsDeclareProperty(Function) \
	static void Function(v8::Local<v8::String> Name, const v8::PropertyCallbackInfo<v8::Value>& Info); \
	static void _ ## Function(v8::Local<v8::String> Name, const v8::PropertyCallbackInfo<v8::Value>& Info) { \
	   v8::Isolate* Isolate = v8::Isolate::GetCurrent(); \
	   v8::HandleScope HandleScope(Isolate); \
	   try { \
	      Function(Name, Info); \
	   } catch (const PExcept& Except) { \
	      /* if(typeid(Except) == typeid(TQmExcept::New(""))) { */ \
            Isolate->ThrowException(v8::Exception::TypeError( \
               v8::String::NewFromUtf8(Isolate, "[la addon] Exception"))); \
         /* } else { \
            throw Except; \
         } */ \
	   } \
	}

#define JsDeclareFunction(Function) \
   static void Function(const v8::FunctionCallbackInfo<v8::Value>& Args); \
   static void _ ## Function(const v8::FunctionCallbackInfo<v8::Value>& Args) { \
      v8::Isolate* Isolate = v8::Isolate::GetCurrent(); \
      v8::HandleScope HandleScope(Isolate); \
      try { \
         Function(Args); \
      } catch (const PExcept& Except) { \
         /* if(typeid(Except) == typeid(TQmExcept::New(""))) { */ \
            Isolate->ThrowException(v8::Exception::TypeError( \
               v8::String::NewFromUtf8(Isolate, "[la addon] Exception"))); \
         /* } else { \
            throw Except; \
         } */ \
      } \
   }

#define QmAssert(Cond) \
	((Cond) ? static_cast<void>(0) : throw TQm::TQmExcept::New(TStr(__FILE__) + " line " + TInt::GetStr(__LINE__) + ": " + TStr(#Cond)))

#define QmAssertR(Cond, MsgStr) \
	((Cond) ? static_cast<void>(0) : throw TQm::TQmExcept::New(MsgStr, TStr(__FILE__) + " line " + TInt::GetStr(__LINE__) + ": " + TStr(#Cond)))

#define QmFailR(MsgStr) \
   Args.GetReturnValue().Set(Isolate->ThrowException(v8::Exception::TypeError( \
         v8::String::NewFromUtf8(Isolate, TStr(MsgStr).CStr())))); return;


//////////////////////////////////////////////////////
// Node - Utilities
class TNodeJsUtil {
public:
   /// Extract argument ArgN property as bool
	static bool GetArgBool(const v8::FunctionCallbackInfo<v8::Value>& Args, const int& ArgN, const TStr& Property, const bool& DefVal) {
		v8::Isolate* Isolate = v8::Isolate::GetCurrent();
		v8::HandleScope HandleScope(Isolate);
		if (Args.Length() > ArgN) {
			if (Args[ArgN]->IsObject() && Args[ArgN]->ToObject()->Has(v8::String::NewFromUtf8(Isolate, Property.CStr()))) {
				v8::Handle<v8::Value> Val = Args[ArgN]->ToObject()->Get(v8::String::NewFromUtf8(Isolate, Property.CStr()));
				 EAssertR(Val->IsBoolean(),
				   TStr::Fmt("Argument %d, property %s expected to be boolean", ArgN, Property.CStr()).CStr());
				 return static_cast<bool>(Val->BooleanValue());
			}
		}
		return DefVal;
	}
   static int GetArgInt32(const v8::FunctionCallbackInfo<v8::Value>& Args, const int& ArgN, const TStr& Property, const int& DefVal) {
		v8::Isolate* Isolate = v8::Isolate::GetCurrent();
		v8::HandleScope HandleScope(Isolate);
		if (Args.Length() > ArgN) {
			if (Args[ArgN]->IsObject() && Args[ArgN]->ToObject()->Has(v8::String::NewFromUtf8(Isolate, Property.CStr()))) {
				v8::Handle<v8::Value> Val = Args[ArgN]->ToObject()->Get(v8::String::NewFromUtf8(Isolate, Property.CStr()));
				 EAssertR(Val->IsInt32(),
				   TStr::Fmt("Argument %d, property %s expected to be int32", ArgN, Property.CStr()).CStr());
				 return Val->ToNumber()->Int32Value();
			}
		}
		return DefVal;
	}

   /// Extract argument ArgN as TStr
   static TStr GetArgStr(const v8::FunctionCallbackInfo<v8::Value>& Args, const int& ArgN) {
	   v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	   v8::HandleScope HandleScope(Isolate);
	   QmAssertR(Args.Length() > ArgN, TStr::Fmt("Missing argument %d", ArgN));
	   v8::Handle<v8::Value> Val = Args[ArgN];
	   QmAssertR(Val->IsString(), TStr::Fmt("Argument %d expected to be string", ArgN));
	   v8::String::Utf8Value Utf8(Val);
	   return TStr(*Utf8);
   }

   /// Extract argument ArgN as TStr, and use DefVal in case when not present
   static TStr GetArgStr(const v8::FunctionCallbackInfo<v8::Value>& Args, const int& ArgN, const TStr& DefVal) {
	   v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	   v8::HandleScope HandleScope(Isolate);
	   if (Args.Length() > ArgN) {
		   v8::Handle<v8::Value> Val = Args[ArgN];
		   QmAssertR(Val->IsString(), TStr::Fmt("Argument %d expected to be string", ArgN));
		   v8::String::Utf8Value Utf8(Val);
		   return TStr(*Utf8);
	   }
	   return DefVal;
   }

   /// Extract argument ArgN property as string
   static TStr GetArgStr(const v8::FunctionCallbackInfo<v8::Value>& Args, const int& ArgN, const TStr& Property, const TStr& DefVal) {
	   v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	   v8::HandleScope HandleScope(Isolate);
	   if (Args.Length() > ArgN) {
		   if (Args[ArgN]->IsObject() && Args[ArgN]->ToObject()->Has(v8::String::NewFromUtf8(Isolate, Property.CStr()))) {
			   v8::Handle<v8::Value> Val = Args[ArgN]->ToObject()->Get(v8::String::NewFromUtf8(Isolate, Property.CStr()));
			   QmAssertR(Val->IsString(), TStr::Fmt("Argument %d, property %s expected to be string", ArgN, Property.CStr()));
			   v8::String::Utf8Value Utf8(Val);
			   return TStr(*Utf8);
		   }
	   }
	   return DefVal;
   }

	// gets the class name of the underlying glib object. the name is stored in an hidden variable "class"
	static TStr GetClass(const v8::Handle<v8::Object> Obj) {
		v8::Isolate* Isolate = v8::Isolate::GetCurrent();
		v8::HandleScope HandleScope(Isolate);
		v8::Local<v8::Value> ClassNm = Obj->GetHiddenValue(v8::String::NewFromUtf8(Isolate, "class"));
		const bool EmptyP = ClassNm.IsEmpty();
		if (EmptyP) { return ""; }
		v8::String::Utf8Value Utf8(ClassNm);
		return TStr(*Utf8);
	}

	// checks if the class name of the underlying glib object matches the given string. the name is stored in an hidden variable "class"
	static bool IsClass(const v8::Handle<v8::Object> Obj, const TStr& ClassNm) {
		TStr ObjClassStr = GetClass(Obj);
		return ObjClassStr == ClassNm;
	}
	/// Check if argument ArgN belongs to a given class
	static bool IsArgClass(const v8::FunctionCallbackInfo<v8::Value>& Args, const int& ArgN, const TStr& ClassNm) {
		v8::Isolate* Isolate = v8::Isolate::GetCurrent();
		v8::HandleScope HandleScope(Isolate);
		EAssertR(Args.Length() > ArgN, TStr::Fmt("Missing argument %d of class %s", ArgN, ClassNm.CStr()).CStr());
      EAssertR(Args[ArgN]->IsObject(), TStr("Argument expected to be '" + ClassNm + "' but is not even an object!").CStr());
		v8::Handle<v8::Value> Val = Args[ArgN];
	 	v8::Handle<v8::Object> Data = v8::Handle<v8::Object>::Cast(Val);
		TStr ClassStr = GetClass(Data);
		return ClassStr.EqI(ClassNm);
	}
};

#endif
