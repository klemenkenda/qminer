/**
 * Copyright (c) 2015, Jozef Stefan Institute, Quintelligence d.o.o. and contributors
 * All rights reserved.
 * 
 * This source code is licensed under the FreeBSD license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "qminer_gs.h"

namespace TQm {

namespace TStorage {

///////////////////////////////
// Store window description
TStr TStoreWndDesc::SysInsertedAtFieldName = "_sys_inserted_at";

void TStoreWndDesc::Save(TSOut& SOut) const {
	TInt(WindowType).Save(SOut);
	WindowSize.Save(SOut);
	InsertP.Save(SOut);
	TimeFieldNm.Save(SOut);
}

void TStoreWndDesc::Load(TSIn& SIn) {
	WindowType = TStoreWndType(TInt(SIn).Val);
	WindowSize.Load(SIn);
	InsertP.Load(SIn);
	TimeFieldNm.Load(SIn);
}

///////////////////////////////
/// Store schema definition.
TStoreSchema::TMaps TStoreSchema::Maps;

TStoreSchema::TMaps::TMaps() {
	// field-type map
	FieldTypeMap.AddDat("int") = oftInt;
	FieldTypeMap.AddDat("int_v") = oftIntV;
	FieldTypeMap.AddDat("uint64") = oftUInt64;
	FieldTypeMap.AddDat("string") = oftStr;
	FieldTypeMap.AddDat("string_v") = oftStrV;
	FieldTypeMap.AddDat("bool") = oftBool;
	FieldTypeMap.AddDat("float") = oftFlt;
	FieldTypeMap.AddDat("float_pair") = oftFltPr;
	FieldTypeMap.AddDat("float_v") = oftFltV;
	FieldTypeMap.AddDat("datetime") = oftTm;
	FieldTypeMap.AddDat("num_sp_v") = oftNumSpV;
	FieldTypeMap.AddDat("bow_sp_v") = oftBowSpV;

	// time-window units
	TimeWindowUnitMap.AddDat("second",                             1000);
	TimeWindowUnitMap.AddDat("minute",                        60 * 1000);
	TimeWindowUnitMap.AddDat("hour",                     60 * 60 * 1000);
	TimeWindowUnitMap.AddDat("day",                 24 * 60 * 60 * 1000);
	TimeWindowUnitMap.AddDat("week",            7 * 24 * 60 * 60 * 1000);
	TimeWindowUnitMap.AddDat("month",  uint64(30) * 24 * 60 * 60 * 1000);
	TimeWindowUnitMap.AddDat("year",  uint64(365) * 24 * 60 * 60 * 1000);
}

TFieldDesc TStoreSchema::ParseFieldDesc(const PJsonVal& FieldVal) {
	// assert necessary stuff there
	QmAssertR(FieldVal->IsObjKey("name"), "Missing field name");
	QmAssertR(FieldVal->IsObjKey("type"), "Missing field type");
	// parse out
	TStr FieldName = FieldVal->GetObjKey("name")->GetStr();
	TStr FieldTypeStr = FieldVal->GetObjKey("type")->GetStr();
	const bool NullableP = FieldVal->GetObjBool("null", false);
	const bool PrimaryP = FieldVal->GetObjBool("primary", false);
	// validate
	TValidNm::AssertValidNm(FieldName);
	QmAssertR(Maps.FieldTypeMap.IsKey(FieldTypeStr), "Unsupported field type " + FieldTypeStr);
	// map field type to enum
	TFieldType FieldType = (TFieldType)Maps.FieldTypeMap.GetDat(FieldTypeStr).Val;
	// done
	return TFieldDesc(FieldName, FieldType,  PrimaryP, NullableP, false);
}

TFieldDescEx TStoreSchema::ParseFieldDescEx(const PJsonVal& FieldVal) {
	TFieldDescEx FieldDescEx;
	// parse flags
	FieldDescEx.SmallStringP = FieldVal->GetObjBool("shortstring", false);
	FieldDescEx.CodebookP = FieldVal->GetObjBool("codebook", false);
	// load default value (if available)
	if (FieldVal->IsObjKey("default")) {
		FieldDescEx.DefaultVal = FieldVal->GetObjKey("default");
	}
	// get storage place (cache or memory)
	TStr StoreLocStr = FieldVal->GetObjStr("store", "memory"); // default is store in memory
	if (StoreLocStr == "memory") {
		FieldDescEx.FieldStoreLoc = slMemory;
	} else if (StoreLocStr == "cache") {
		FieldDescEx.FieldStoreLoc = slDisk;
	} else {
		throw TQmExcept::New(TStr::Fmt("Unsupported 'store' flag for field: %s", StoreLocStr.CStr()));
	}
	// done
	return FieldDescEx;
}

TJoinDescEx TStoreSchema::ParseJoinDescEx(const PJsonVal& JoinVal) {
	// assert necessary stuff there
	QmAssertR(JoinVal->IsObjKey("name"), "Missing join name");
	QmAssertR(JoinVal->IsObjKey("type"), "Missing join type");
	QmAssertR(JoinVal->IsObjKey("store"), "Missing join store");
	// parse parameters
	TStr JoinName = JoinVal->GetObjKey("name")->GetStr();
	TStr JoinType = JoinVal->GetObjKey("type")->GetStr();
	TStr JoinStore = JoinVal->GetObjKey("store")->GetStr();
	// get extra description
	TJoinDescEx JoinDescEx;
	JoinDescEx.JoinName = JoinName;
	JoinDescEx.JoinStoreName = JoinStore;
	// get join type
	if (JoinType == "index") {
		JoinDescEx.JoinType = osjtIndex;
	} else if (JoinType == "field") {
		JoinDescEx.JoinType = osjtField;
	} else {
		throw TQmExcept::New("Unsupported join type");
	}
	// get inverse join
	if (JoinVal->IsObjKey("inverse")){
		JoinDescEx.InverseJoinName = JoinVal->GetObjKey("inverse")->GetStr();
	}
	// get "is small" flag
	if (JoinVal->IsObjKey("small")) {
		JoinDescEx.IsSmall = JoinVal->GetObjKey("small")->GetBool();
	}
	// done
	return JoinDescEx;
}

TIndexKeyEx TStoreSchema::ParseIndexKeyEx(const PJsonVal& IndexKeyVal) {
	// check for mandatory fields
	QmAssertR(IndexKeyVal->IsObjKey("field"), "Missing key-index field");
	QmAssertR(IndexKeyVal->IsObjKey("type"), "Missing key-index type");
	// parse out indexed field
	TIndexKeyEx IndexKeyEx;
	IndexKeyEx.FieldName = IndexKeyVal->GetObjStr("field");
	// check if it is a valid field name
	QmAssertR(FieldH.IsKey(IndexKeyEx.FieldName), "Target field for key-index unknown");
	// get field type to avoid further lookups when indexing
	TFieldType FieldType = FieldH.GetDat(IndexKeyEx.FieldName).GetFieldType();
	// parse out key name, use field name as default
	IndexKeyEx.KeyIndexName = IndexKeyVal->GetObjStr("name", IndexKeyEx.FieldName);
	// get and parse key type
	TStr KeyTypeStr = IndexKeyVal->GetObjStr("type");
	if (KeyTypeStr == "value") {
		IndexKeyEx.KeyType = oiktValue;
	} else if (KeyTypeStr == "text") {
		IndexKeyEx.KeyType = oiktText;
	} else if (KeyTypeStr == "location") {
		IndexKeyEx.KeyType = oiktLocation;
	} else {
		throw TQmExcept::New("Unknown key type " +  KeyTypeStr);
	}
	// check if small index should be used
	if (IndexKeyVal->IsObjKey("small") && IndexKeyVal->GetObjBool("small")) {
		IndexKeyEx.KeyType = (TIndexKeyType)(IndexKeyEx.KeyType | oiktSmall);
	}
	// check field type and index type match
	if (FieldType == oftStr && IndexKeyEx.IsValue()) {
	} else if (FieldType == oftStr && IndexKeyEx.IsText()) {
	} else if (FieldType == oftStrV && IndexKeyEx.IsValue()) {
	} else if (FieldType == oftTm && IndexKeyEx.IsValue()) {
	} else if (FieldType == oftFltPr && IndexKeyEx.IsLocation()) {
	} else {
		// not supported, lets complain about it...
		throw TQmExcept::New("Indexing '" + KeyTypeStr + "' not supported for field " + IndexKeyEx.FieldName);
	}
	// get and parse sort type
	if (IndexKeyVal->IsObjKey("sort")) {
		TStr SortTypeStr = IndexKeyVal->GetObjStr("sort");
		if (SortTypeStr == "string") {
			IndexKeyEx.SortType = oikstByStr;
		} else if (SortTypeStr == "id") {
			IndexKeyEx.SortType = oikstById;
		} else if (SortTypeStr == "number") {
			IndexKeyEx.SortType = oikstByFlt;
		} else {
			throw TQmExcept::New("Unsupported sort type " + SortTypeStr);
		}
	} else {
		IndexKeyEx.SortType = oikstUndef;
	}
	// parse out word vocabulary
	IndexKeyEx.WordVocName = IndexKeyVal->GetObjStr("vocabulary", "");
	// parse out tokenizer
	if (IndexKeyEx.IsText()) {
		if (IndexKeyVal->IsObjKey("tokenizer")) {
			PJsonVal TokenizerVal = IndexKeyVal->GetObjKey("tokenizer");
			QmAssertR(TokenizerVal->IsObjKey("type"), 
				"Missing tokenizer type " + TokenizerVal->SaveStr());
			const TStr& TypeNm = TokenizerVal->GetObjStr("type");
			IndexKeyEx.Tokenizer = TTokenizer::New(TypeNm, TokenizerVal);
		} else {
			IndexKeyEx.Tokenizer = TTokenizers::THtmlUnicode::New();
		}
	}
	return IndexKeyEx;
}

TStoreSchema::TStoreSchema(const PJsonVal& StoreVal): StoreId(0), HasStoreIdP(false) {
	QmAssertR(StoreVal->IsObj(), "Invalid JSON for store definition.");
	// get store name
	QmAssertR(StoreVal->IsObjKey("name"), "Missing store name.");
	StoreName = StoreVal->GetObjStr("name");
	BlockSizeMem = 1000;
	// get additional options
	if (StoreVal->IsObjKey("options")) {
		PJsonVal options = StoreVal->GetObjKey("options");
		if (options->IsObjKey("type")) {
			StoreType = options->GetObjStr("type");
		}
		// parse block size
		BlockSizeMem = MAX(1, options->GetObjInt("block_size_mem", BlockSizeMem));
	}
	// get id (optional)
	if (StoreVal->IsObjKey("id")) {
		const int _StoreId = StoreVal->GetObjInt("id");
		QmAssertR(_StoreId >= 0 && _StoreId < (int)TEnv::GetMxStores(), "Store " + StoreName + " ID out of range");
		StoreId = (uint)_StoreId;
		HasStoreIdP = true;
	}
	// fields
	QmAssertR(StoreVal->IsObjKey("fields"), "Missing field list.");
	PJsonVal FieldDefs = StoreVal->GetObjKey("fields");
	QmAssertR(FieldDefs->IsArr(), "Bad field list.");

	// parser fields
	for (int FieldN = 0; FieldN < FieldDefs->GetArrVals(); FieldN++) {
		PJsonVal FieldDef = FieldDefs->GetArrVal(FieldN);
		// prase basic field description
		TFieldDesc FieldDesc = ParseFieldDesc(FieldDef);
		QmAssertR(!FieldH.IsKey(FieldDesc.GetFieldNm()), "Duplicate field name " + FieldDesc.GetFieldNm() + " in store " + StoreName);
		FieldH.AddDat(FieldDesc.GetFieldNm(), FieldDesc);
		// prase extended field description required for serialization
		TFieldDescEx FieldDescEx = ParseFieldDescEx(FieldDef);
		FieldExH.AddDat(FieldDesc.GetFieldNm(), FieldDescEx);
	}

	// parse keys
	if (StoreVal->IsObjKey("keys")) {
		PJsonVal KeyDefs = StoreVal->GetObjKey("keys");
		QmAssertR(KeyDefs->IsArr(), "Bad key list.");
		for (int KeyN = 0; KeyN < KeyDefs->GetArrVals(); KeyN++) {
			PJsonVal KeyDef = KeyDefs->GetArrVal(KeyN);
			TIndexKeyEx IndexKeyDesc = ParseIndexKeyEx(KeyDef);
			IndexKeyExV.Add(IndexKeyDesc);
		}
	}

	// joins
	if (StoreVal->IsObjKey("joins")) {
		PJsonVal JoinDefs = StoreVal->GetObjKey("joins");
		QmAssertR(JoinDefs->IsArr(), "Bad join list.");
		for (int JoinN = 0; JoinN < JoinDefs->GetArrVals(); JoinN++) {
			PJsonVal JoinDef = JoinDefs->GetArrVal(JoinN);
			// parse join
			TJoinDescEx JoinDescEx = ParseJoinDescEx(JoinDef);
			// add new field in case of field join
			if (JoinDescEx.JoinType == osjtField) {
				// we create two fields for each field join: record Id and frequency
				TStr JoinRecFieldNm = JoinDescEx.JoinName + "Id";
				TStr JoinFqFieldNm = JoinDescEx.JoinName + "Fq";
				// prepare join field descriptions
				FieldH.AddDat(JoinRecFieldNm, TFieldDesc(JoinRecFieldNm, oftUInt64, false, true, true));
				FieldH.AddDat(JoinFqFieldNm, TFieldDesc(JoinFqFieldNm, oftInt, false, true, true));
				// prepare extended field description
				FieldExH.AddDat(JoinRecFieldNm, TFieldDescEx(slMemory, false, false));
				FieldExH.AddDat(JoinFqFieldNm, TFieldDescEx(slMemory, false, false));
			}
			// remember join
			JoinDescExV.Add(JoinDescEx);
		}
	}
	
	// parse window size
	if (StoreVal->IsObjKey("window")) {
		// window size defined in number of records
		WndDesc.WindowType = swtLength;
		// parse size
		PJsonVal WindowSize = StoreVal->GetObjKey("window");
		QmAssertR(WindowSize->IsNum(), "Bad window size parameter.");
		WndDesc.WindowSize = (uint64)WindowSize->GetInt();
	} else if (StoreVal->IsObjKey("timeWindow")) {
		// time-defined window, parse out details
		WndDesc.WindowType = swtTime;
		// parse parameters
		PJsonVal TimeWindow = StoreVal->GetObjKey("timeWindow");
		QmAssertR(TimeWindow->IsObj(), "Bad timeWindow parameter.");
		// get window duration
		QmAssertR(TimeWindow->IsObjKey("duration"), "Missing duration parameter.");
		uint64 WindowSize = (uint64)(TimeWindow->GetObjInt("duration"));
		// get duration unit
		TStr UnitStr = TimeWindow->GetObjStr("unit", "second");
		// check we know of the unit
		QmAssertR(Maps.TimeWindowUnitMap.IsKey(UnitStr),
			"Unsupported timeWindow length unit type: " + UnitStr);
		// set time duration in milliseconds
		const uint64 FactorMSecs = Maps.TimeWindowUnitMap.GetDat(UnitStr);
		WndDesc.WindowSize = WindowSize * FactorMSecs;
		// get field giving the tact for time
		if (TimeWindow->IsObjKey("field")) {
			WndDesc.TimeFieldNm = TimeWindow->GetObjStr("field");
			WndDesc.InsertP = false;
		} else {
			// no time field, create one which takes insert-time value
			TFieldDesc FieldDesc(TStoreWndDesc::SysInsertedAtFieldName, oftTm, false, false, true);
			FieldH.AddDat(FieldDesc.GetFieldNm(), FieldDesc);

			TFieldDescEx FieldDescEx;
			FieldDescEx.FieldStoreLoc = slDisk;
			FieldDescEx.SmallStringP = false;
			FieldDescEx.CodebookP = false;
			FieldExH.AddDat(FieldDesc.GetFieldNm(), FieldDescEx);

			WndDesc.TimeFieldNm = FieldDesc.GetFieldNm();
			WndDesc.InsertP = true;
		}
	}
}

void TStoreSchema::ParseSchema(const PJsonVal& SchemaVal, TStoreSchemaV& SchemaV) {
	if (SchemaVal->IsArr()) {
		for (int SchemaN = 0; SchemaN < SchemaVal->GetArrVals(); SchemaN++) {
			SchemaV.Add(TStoreSchema(SchemaVal->GetArrVal(SchemaN)));
		}
	} else if (SchemaVal->IsObjKey("stores")) {
		ParseSchema(SchemaVal->GetObjKey("stores"), SchemaV);
	} else {
		SchemaV.Add(TStoreSchema(SchemaVal));
	}
}

void TStoreSchema::ValidateSchema(const TWPt<TBase>& Base, TStoreSchemaV& SchemaV) {
	TStrH StoreNameH;
	TUIntV ReqStoreIdV;

	for (int SchemaN = 0; SchemaN < SchemaV.Len(); SchemaN++){
		TStoreSchema& Schema = SchemaV[SchemaN];
		// unique store names
		TStr StoreName = Schema.StoreName;
		QmAssertR(!Base->IsStoreNm(StoreName), "Store already exists: " + StoreName);
		QmAssertR(!StoreNameH.IsKey(StoreName), "Duplicate store name: " + StoreName);
		StoreNameH.AddDat(StoreName, 0);
		// check unique store ids
		if (SchemaV[SchemaN].HasStoreIdP) {
			uint ReqStoreId = Schema.StoreId;
			for (int ReqStoreIdN = 0; ReqStoreIdN < ReqStoreIdV.Len(); ReqStoreIdN++) {
				QmAssertR(ReqStoreId != ReqStoreIdV[ReqStoreIdN],
					"Duplicate store id " + TInt::GetStr(ReqStoreId));
			}
			ReqStoreIdV.Add(ReqStoreId);
		}
		// valid field names inside the store and primary key
		TStr PrimaryFieldName;
		int FieldKeyId = Schema.FieldH.FFirstKeyId();
		while (Schema.FieldH.FNextKeyId(FieldKeyId)) {
			TStr FieldName = Schema.FieldH[FieldKeyId].GetFieldNm();
			const TFieldDesc& FieldDesc = Schema.FieldH[FieldKeyId];
			TValidNm::AssertValidNm(FieldName);
			// determine primary field matches constraints
			if (FieldDesc.IsPrimary()) {
				// more than one field is marked as "primary"
				QmAssertR(PrimaryFieldName.Empty(), "More than one field is marked as primary: " + PrimaryFieldName + " and " + FieldDesc.GetFieldNm());
				// fields marked as "primary" must be strings
				QmAssertR(FieldDesc.IsStr() || FieldDesc.IsInt() || FieldDesc.IsUInt64() || FieldDesc.IsFlt() || FieldDesc.IsTm(),
					"Field marked as primary must be of type string, int, uint64, float or time: " + FieldDesc.GetFieldNm());
				// fields marked as "primary" must be strings
				QmAssertR(!FieldDesc.IsNullable(), "Filed marked as primary cannot be nullable: " + FieldDesc.GetFieldNm());
				// all fine, mark it as primary
				PrimaryFieldName = FieldName;
			}
		}
		
		// check that window parameter for field is valid
		if (Schema.WndDesc.WindowType == swtTime) {
			const TStr& WndFieldName = Schema.WndDesc.TimeFieldNm;
			QmAssertR(Schema.FieldH.IsKey(WndFieldName), "Field " + WndFieldName +
				" should be used as the source for window is store " + StoreName +
				", but it doesn't exist.");
			const TFieldDesc& FieldDesc = Schema.FieldH.GetDat(WndFieldName);
			QmAssertR(FieldDesc.IsTm(), "Field " + WndFieldName +
				" should be used as the source for window is store " + StoreName +
				", but it is not of datetime type.");
			QmAssertR(!FieldDesc.IsNullable(), "Field " + WndFieldName +
				" should be used as the source for window is store " + StoreName +
				", but it is nullable.");
		}
		// joins
		TStrH JoinNameH;
		for (int JoinN = 0; JoinN < Schema.JoinDescExV.Len(); JoinN++){
			TStr JoinName = Schema.JoinDescExV[JoinN].JoinName;
			QmAssertR(!JoinNameH.IsKey(JoinName),
				"Duplicate join name " + JoinName + " in store " + StoreName);
			JoinNameH.AddKey(JoinName);
			// check if the other store exists
			TStr JoinStoreName = Schema.JoinDescExV[JoinN].JoinStoreName;
			// check if a store exists with that name in the schemas given
			bool FoundP = false;
			for (int SchemaN = 0; SchemaN < SchemaV.Len(); SchemaN++){
				if (SchemaV[SchemaN].StoreName == JoinStoreName){
					FoundP = true;
					break;
				}
			}
			// check for an existing store with that name
			if(!FoundP) {
				for (int StoreN = 0; StoreN < Base->GetStores(); StoreN++){
					if (Base->GetStoreByStoreN(StoreN)->GetStoreNm() == JoinStoreName) {
						FoundP = true;
						break;
					}
				}
			}
			QmAssertR(FoundP, "Illegal join " + JoinName + " in store " +
				StoreName + " - joined store " + JoinStoreName + " not found");
		}
	}
}

///////////////////////////////
// In-memory storage
TInMemStorage::TInMemStorage(const TStr& _FNm, const int& _BlockSize) : FNm(_FNm), BlobFNm(_FNm + "Blob"), Access(faCreate), BlockSize(_BlockSize) {
	BlobStorage = TMBlobBs::New(BlobFNm, Access);
}

TInMemStorage::TInMemStorage(const TStr& _FNm, const TFAccess& _Access, const bool& _Lazy) :
	FNm(_FNm), BlobFNm(_FNm + "Blob"), Access(_Access) {
	
	// load data
	TFIn FIn(FNm); 
	BlobPtV.Load(FIn); // load vector
	// load rest
	TInt64 cnt;
	cnt.Load(FIn);
	FirstValOffset.Load(FIn);
	FirstValOffsetMem.Load(FIn);
	BlockSize.Load(FIn);

	// load data from blob storage
	BlobStorage = TMBlobBs::New(BlobFNm, Access);

	for (int64 i = 0; i < cnt; i++) {
		ValV.Add(); // empty (non-loaded) data
		DirtyV.Add(isdfNotLoaded); // init dirty flags
	}
	if (!_Lazy) {
		LoadAll();
	}
}

TInMemStorage::~TInMemStorage() {
	if (Access != faRdOnly) {
		// store dirty vectors
		for (int i = 0; i < ValV.Len(); i++) {
			SaveRec(i);
		}		
		// save vector
		TFOut FOut(FNm); 
		BlobPtV.Save(FOut);
		// save rest
		TInt64(ValV.Len()).Save(FOut);
		FirstValOffset.Save(FOut);
		FirstValOffsetMem.Save(FOut);
		BlockSize.Save(FOut);
	}
}

/// Utility method for loading specific record
void TInMemStorage::LoadRec(int64 RecN) const {
	if (DirtyV[RecN] != isdfNotLoaded) {
		return;
	}
	const int64 ii = RecN / BlockSize;
	TMem mem;
	TMem::LoadMem(BlobStorage->GetBlob(BlobPtV[ii]), mem);
	PSIn in = mem.GetSIn();
	for (int64 j = ii*BlockSize; j < DirtyV.Len() && j < (ii + 1)*BlockSize; j++) {
		if (DirtyV[j] == isdfNotLoaded) {
			DirtyV[j] = isdfClean;
			ValV[j].Load(in);
		} else {
			TMem mem2;
			mem2.Load(in);
		}
	}
}

/// Utility method for storing specific record
int TInMemStorage::SaveRec(int RecN) {
	int res = 0;
	switch (DirtyV[RecN]) {
	case isdfNew:
	case isdfDirty:
		{
			res++;
			const int ii = RecN / BlockSize;
			TMOut mem;
			for (int j = ii*BlockSize; j < DirtyV.Len() && j < (ii + 1)*BlockSize; j++) {
				ValV[j].Save(mem);
				DirtyV[j] = isdfClean;
			}
			while (BlobPtV.Len() <= ii) {
				BlobPtV.Add();
			}
			if (BlobPtV[ii].Empty()) {
				BlobPtV[ii] = BlobStorage->PutBlob(mem.GetSIn());
			} else {
				BlobPtV[ii] = BlobStorage->PutBlob(BlobPtV[ii], mem.GetSIn());
			}
		}
		break;
	case isdfClean:
	case isdfNotLoaded: break;
	}
	return res;
}

void TInMemStorage::AssertReadOnly() const {
	QmAssertR(((Access == faCreate) || (Access == faUpdate)), FNm + " opened in Read-Only mode!");
}

bool TInMemStorage::IsValId(const uint64& ValId) const {
	return 
		(ValId >= FirstValOffsetMem.Val + FirstValOffset) &&
		(ValId < FirstValOffsetMem.Val + ValV.Len());
}

void TInMemStorage::GetVal(const uint64& ValId, TMem& Val) const {
	uint64 i = ValId - FirstValOffsetMem;
	LoadRec(i);
	Val = ValV[i];
}

uint64 TInMemStorage::AddVal(const TMem& Val) {
	uint64 res = ValV.Add(Val);
	DirtyV.Add(isdfNew);
	if (ValV.Len() % BlockSize == 1) {
		BlobPtV.Add();
	}
	return res + FirstValOffsetMem;
}

void TInMemStorage::SetVal(const uint64& ValId, const TMem& Val) {
	AssertReadOnly();
	ValV[ValId - FirstValOffsetMem] = Val;
	uchar& flag = DirtyV[ValId - FirstValOffsetMem];
	if (flag == isdfNew) { } // new remains new
	else { flag = isdfDirty; } // set as dirty
}

void TInMemStorage::DelVals(int Vals) {
	if (Vals > 0) {
		int ValsTrue = 0;
		for (ValsTrue = 0; ValsTrue < Vals && ValsTrue<ValV.Len(); ValsTrue++) {
			ValV[ValsTrue + FirstValOffset].Clr();
		}
		int blocks_to_delete = ((int)FirstValOffset + ValsTrue) / BlockSize;
		int vals_to_delete = blocks_to_delete * BlockSize;

		if (vals_to_delete > 0) {
			ValV.Del(0, vals_to_delete - 1);
			DirtyV.Del(0, vals_to_delete - 1);
			for (int i = 0; i < blocks_to_delete; i++) {
				if (!BlobPtV[i].Empty()) {
					BlobStorage->DelBlob(BlobPtV[i]);
				}
			}
			BlobPtV.Del(0, blocks_to_delete - 1);
		}
		FirstValOffset += ValsTrue - vals_to_delete;
		FirstValOffsetMem += vals_to_delete;
	}
}

uint64 TInMemStorage::Len() const {
	return ValV.Len() - FirstValOffset;
}

uint64 TInMemStorage::GetFirstValId() const {
	return FirstValOffsetMem + FirstValOffset;
}

uint64 TInMemStorage::GetLastValId() const {
	return FirstValOffsetMem + ValV.Len() - 1;
}

int TInMemStorage::PartialFlush(int WndInMsec) {
	TTmStopWatch sw(true);
	int res = 0;
	for (int i = 0; i< ValV.Len(); i++) {
		if (sw.GetMSecInt() > WndInMsec) 
			break;
		res += SaveRec(i);
	}
	return res;
}

void TInMemStorage::LoadAll() {
	for (int i = 0; i < ValV.Len(); i++) {
		LoadRec(i);
	}
}

///////////////////////////////
// Field serialization parameters
void TRecSerializator::TFieldSerialDesc::Save(TSOut& SOut) const {
	FieldId.Save(SOut);
	TInt(StoreLoc).Save(SOut);
	NullMapByte.Save(SOut);
	NullMapMask.Save(SOut);
	FixedPartP.Save(SOut);
	Offset.Save(SOut);
	CodebookP.Save(SOut);
	SmallStringP.Save(SOut);
	DefaultVal.Save(SOut);
}

void TRecSerializator::TFieldSerialDesc::Load(TSIn& SIn) {
	FieldId.Load(SIn);
	StoreLoc = TStoreLoc(TInt(SIn).Val);
	NullMapByte = TUCh(SIn);
	NullMapMask = TUCh(SIn);
	FixedPartP.Load(SIn);
	Offset.Load(SIn);
	CodebookP.Load(SIn);
	SmallStringP.Load(SIn);
	DefaultVal = PJsonVal(SIn);
}


///////////////////////////////
// Serialization and de-serialization of records to TMem

TStr TRecSerializator::GetErrorMsg(const TMem& RecMem, const TFieldSerialDesc& FieldSerialDesc) const {
	return TStr::Fmt("FPO:%d VIPO:%d VCPO:%d|L:%d|FID:%d NMP:%d NMM:%d FP:%s O:%d",
		FixedPartOffset.Val, VarIndexPartOffset.Val, VarContentPartOffset.Val,
		RecMem.Len(), FieldSerialDesc.FieldId.Val, (int)FieldSerialDesc.NullMapByte.Val,
		(int)FieldSerialDesc.NullMapMask.Val, FieldSerialDesc.FixedPartP ? "T" : "F",
		FieldSerialDesc.Offset);
}
TStr TRecSerializator::GetErrorMsg(const char* Bf, const int& BfL, const TFieldSerialDesc& FieldSerialDesc) const {
	return TStr::Fmt("FPO:%d VIPO:%d VCPO:%d|L:%d|FID:%d NMP:%d NMM:%d FP:%s O:%d",
		FixedPartOffset.Val, VarIndexPartOffset.Val, VarContentPartOffset.Val,
		BfL, FieldSerialDesc.FieldId.Val, (int)FieldSerialDesc.NullMapByte.Val,
		(int)FieldSerialDesc.NullMapMask.Val, FieldSerialDesc.FixedPartP ? "T" : "F",
		FieldSerialDesc.Offset);
}
const TRecSerializator::TFieldSerialDesc& TRecSerializator::GetFieldSerialDesc(const int& FieldId) const {
	QmAssertR(FieldIdToSerialDescIdH.IsKey(FieldId),
		"Field with ID not found: " + TInt::GetStr(FieldId));
	return FieldSerialDescV[FieldIdToSerialDescIdH.GetDat(FieldId)];
}

//////////////////////

char* TRecSerializator::GetLocationFixed(const TMemBase& RecMem,
		const TFieldSerialDesc& FieldSerialDesc) const {
	return GetLocationFixed(RecMem.GetBf(), RecMem.Len(), FieldSerialDesc);
}

int TRecSerializator::GetOffsetVar(const TMemBase& RecMem,
		const TFieldSerialDesc& FieldSerialDesc) const {
	return GetOffsetVar(RecMem.GetBf(), RecMem.Len(), FieldSerialDesc);
}

char* TRecSerializator::GetLocationVar(const TMemBase& RecMem,
		const TFieldSerialDesc& FieldSerialDesc) const {
	return GetLocationVar(RecMem.GetBf(), RecMem.Len(), FieldSerialDesc);
}

int TRecSerializator::GetVarPartBfLen(const TMemBase& RecMem,
		const TFieldSerialDesc& FieldSerialDesc) {
	return GetVarPartBfLen(RecMem.GetBf(), RecMem.Len(), FieldSerialDesc);
}

//////////////////////////

/// finds location inside the buffer for fixed-width fields
char* TRecSerializator::GetLocationFixed(TThinMIn min, const TFieldSerialDesc& FieldSerialDesc) const {
	return GetLocationFixed(min.GetBfAddrChar(), min.Len(), FieldSerialDesc);
}
/// finds location inside the buffer for variable-width fields
int TRecSerializator::GetOffsetVar(TThinMIn min, const TFieldSerialDesc& FieldSerialDesc) const {
	return GetOffsetVar(min.GetBfAddrChar(), min.Len(), FieldSerialDesc);
}
/// finds location inside the buffer for variable-width fields
char* TRecSerializator::GetLocationVar(TThinMIn min, const TFieldSerialDesc& FieldSerialDesc) const {
	return GetLocationVar(min.GetBfAddrChar(), min.Len(), FieldSerialDesc);
}
/// calculates length of buffer where given var-length field is stored
int TRecSerializator::GetVarPartBfLen(TThinMIn min, const TFieldSerialDesc& FieldSerialDesc) {
	return GetVarPartBfLen(min.GetBfAddrChar(), min.Len(), FieldSerialDesc);
}

/////////////////////////

char* TRecSerializator::GetLocationFixed(char* Bf, const int& BfL,
	const TFieldSerialDesc& FieldSerialDesc) const {

	char* bf = Bf + FixedPartOffset + FieldSerialDesc.Offset;
	AssertR(bf < Bf + BfL, GetErrorMsg(Bf, BfL, FieldSerialDesc));
	return bf;
}

int TRecSerializator::GetOffsetVar(const char* Bf, const int& BfL,
	const TFieldSerialDesc& FieldSerialDesc) const {

	int Offset = *((int*)(Bf + VarIndexPartOffset + FieldSerialDesc.Offset));
	AssertR(VarContentPartOffset + Offset < BfL, GetErrorMsg(Bf, BfL, FieldSerialDesc));
	return VarContentPartOffset + Offset;
}

char* TRecSerializator::GetLocationVar(char* Bf, const int& BfL,
	const TFieldSerialDesc& FieldSerialDesc) const {

	int Offset = *((int*)(Bf + VarIndexPartOffset + FieldSerialDesc.Offset));
	char* bf2 = Bf + VarContentPartOffset + Offset;
	AssertR(bf2 < Bf + BfL, GetErrorMsg(Bf, BfL, FieldSerialDesc));
	return bf2;
}

int TRecSerializator::GetVarPartBfLen(const char* Bf, const int& BfL,
	const TFieldSerialDesc& FieldSerialDesc) {
	const char* bf = (Bf + VarIndexPartOffset.Val + FieldSerialDesc.Offset);
	int Offset1 = *((int*)bf);
	int Offset2 = -1;
	if (VarIndexPartOffset + FieldSerialDesc.Offset + (int)sizeof(int) < VarContentPartOffset) {
		Offset2 = *((int*)(bf + sizeof(int)));
	} else {
		Offset2 = BfL - VarContentPartOffset;
	}
	AssertR(Offset2 - Offset1 >= 0, GetErrorMsg(Bf, BfL, FieldSerialDesc));
	return Offset2 - Offset1;
}

///////////////////////////////

void TRecSerializator::SetLocationVar(TMem& RecMem,
		const TFieldSerialDesc& FieldSerialDesc, const int& VarOffset) const {
	SetLocationVar(RecMem.GetBf(), RecMem.Len(), FieldSerialDesc, VarOffset);
}

void TRecSerializator::SetFieldNull(TMem& RecMem,
		const TFieldSerialDesc& FieldSerialDesc, const bool& NullP) const {
	SetFieldNull(RecMem.GetBf(), RecMem.Len(), FieldSerialDesc, NullP);
}

/// set content offset for specified variable field
void TRecSerializator::SetLocationVar(char* Bf, const int& BfL, const TFieldSerialDesc& FieldSerialDesc, const int& VarOffset) const {
	AssertR(VarIndexPartOffset + FieldSerialDesc.Offset <= (BfL - 4), GetErrorMsg(Bf, BfL, FieldSerialDesc));
	*((int*)(Bf + VarIndexPartOffset + FieldSerialDesc.Offset)) = VarOffset;
}
/// sets or un-sets NULL flag for specified field
void TRecSerializator::SetFieldNull(char* Bf, const int& BfL, const TFieldSerialDesc& FieldSerialDesc, const bool& NullP) const {
	char* bf = Bf + FieldSerialDesc.NullMapByte;
	AssertR(bf < Bf + BfL, GetErrorMsg(Bf, BfL, FieldSerialDesc));
	if (NullP) {
		*bf |= FieldSerialDesc.NullMapMask;
	} else {
		*bf &= ~FieldSerialDesc.NullMapMask;
	}
}


void TRecSerializator::SetFieldInt(TMemBase& RecMem,
		const TFieldSerialDesc& FieldSerialDesc, const int& Int) {
	SetFieldInt(RecMem.GetBf(), RecMem.Len(), FieldSerialDesc, Int);
}

void TRecSerializator::SetFieldUInt64(TMemBase& RecMem,
		const TFieldSerialDesc& FieldSerialDesc, const uint64& UInt64) {
	SetFieldUInt64(RecMem.GetBf(), RecMem.Len(), FieldSerialDesc, UInt64);
}

void TRecSerializator::SetFieldStr(TMemBase& RecMem,
		const TFieldSerialDesc& FieldSerialDesc, const TStr& Str) {
	SetFieldStr(RecMem.GetBf(), RecMem.Len(), FieldSerialDesc, Str);
}

void TRecSerializator::SetFieldBool(TMemBase& RecMem,
		const TFieldSerialDesc& FieldSerialDesc, const bool& Bool) {
	SetFieldBool(RecMem.GetBf(), RecMem.Len(), FieldSerialDesc, Bool);
}

void TRecSerializator::SetFieldFlt(TMemBase& RecMem,
		const TFieldSerialDesc& FieldSerialDesc, const double& Flt) {
	SetFieldFlt(RecMem.GetBf(), RecMem.Len(), FieldSerialDesc, Flt);
}

void TRecSerializator::SetFieldFltPr(TMemBase& RecMem,
		const TFieldSerialDesc& FieldSerialDesc, const TFltPr& FltPr) {
	SetFieldFltPr(RecMem.GetBf(), RecMem.Len(), FieldSerialDesc, FltPr);
}

void TRecSerializator::SetFieldTm(TMemBase& RecMem,
		const TFieldSerialDesc& FieldSerialDesc, const TTm& Tm) {
	SetFieldTm(RecMem.GetBf(), RecMem.Len(), FieldSerialDesc, Tm);
}

void TRecSerializator::SetFieldTmMSecs(TMemBase& RecMem,
		const TFieldSerialDesc& FieldSerialDesc, const uint64& TmMSecs) {
	SetFieldTmMSecs(RecMem.GetBf(), RecMem.Len(), FieldSerialDesc, TmMSecs);
}

void TRecSerializator::SetFixedJsonVal(TMemBase& RecMem,
		const TFieldSerialDesc& FieldSerialDesc, const TFieldDesc& FieldDesc,
		const PJsonVal& JsonVal) {
	SetFixedJsonVal(RecMem.GetBf(), RecMem.Len(), FieldSerialDesc, FieldDesc, JsonVal);
}

void TRecSerializator::SetFieldNull(char* Bf, const int& BfL, const int& FieldId, const bool& NullP) {
	SetFieldNull(Bf, BfL, GetFieldSerialDesc(FieldId), NullP);
}

void TRecSerializator::SetFieldInt(char* Bf, const int& BfL,
	const TFieldSerialDesc& FieldSerialDesc, const int& Int) {

	char* bf = GetLocationFixed(Bf, BfL, FieldSerialDesc);
	*((int*)bf) = Int;
}

void TRecSerializator::SetFieldUInt64(char* Bf, const int& BfL,
	const TFieldSerialDesc& FieldSerialDesc, const uint64& UInt64) {

	char* bf = GetLocationFixed(Bf, BfL, FieldSerialDesc);
	*((uint64*)bf) = UInt64;
}

void TRecSerializator::SetFieldStr(char* Bf, const int& BfL,
	const TFieldSerialDesc& FieldSerialDesc, const TStr& Str) {

	char* bf = GetLocationFixed(Bf, BfL, FieldSerialDesc);
	const int StrId = CodebookH.AddKey(Str);
	*((int*)bf) = StrId;
}

void TRecSerializator::SetFieldBool(char* Bf, const int& BfL,
	const TFieldSerialDesc& FieldSerialDesc, const bool& Bool) {

	char* bf = GetLocationFixed(Bf, BfL, FieldSerialDesc);
	*((bool*)bf) = Bool;
}

void TRecSerializator::SetFieldFlt(char* Bf, const int& BfL,
	const TFieldSerialDesc& FieldSerialDesc, const double& Flt) {

	char* bf = GetLocationFixed(Bf, BfL, FieldSerialDesc);
	*((double*)bf) = Flt;
}

void TRecSerializator::SetFieldFltPr(char* Bf, const int& BfL,
	const TFieldSerialDesc& FieldSerialDesc, const TFltPr& FltPr) {

	char* bf = GetLocationFixed(Bf, BfL, FieldSerialDesc);
	*((double*)bf) = FltPr.Val1.Val;
	*(((double*)bf) + 1) = FltPr.Val2.Val;
}

void TRecSerializator::SetFieldTm(char* Bf, const int& BfL,
	const TFieldSerialDesc& FieldSerialDesc, const TTm& Tm) {

	char* bf = GetLocationFixed(Bf, BfL, FieldSerialDesc);
	uint64 TmMSecs = TTm::GetMSecsFromTm(Tm);
	*((uint64*)bf) = TmMSecs;
}

void TRecSerializator::SetFieldTmMSecs(char* Bf, const int& BfL,
	const TFieldSerialDesc& FieldSerialDesc, const uint64& TmMSecs) {

	char* bf = GetLocationFixed(Bf, BfL, FieldSerialDesc);
	*((uint64*)bf) = TmMSecs;
}

void TRecSerializator::SetFixedJsonVal(char* Bf, const int& BfL,
	const TFieldSerialDesc& FieldSerialDesc, const TFieldDesc& FieldDesc,
	const PJsonVal& JsonVal) {

	// call type-appropriate setter
	switch (FieldDesc.GetFieldType()) {
	case oftInt:
		QmAssertR(JsonVal->IsNum(), "Provided JSon data field " + FieldDesc.GetFieldNm() + " is not numeric.");
		SetFieldInt(Bf, BfL, FieldSerialDesc, JsonVal->GetInt());
		break;
	case oftUInt64:
		QmAssertR(JsonVal->IsNum(), "Provided JSon data field " + FieldDesc.GetFieldNm() + " is not numeric.");
		SetFieldUInt64(Bf, BfL, FieldSerialDesc, (uint64)JsonVal->GetInt());
		break;
	case oftStr:
		// this string should be encoded using a codebook
		QmAssertR(JsonVal->IsStr(), "Provided JSon data field " + FieldDesc.GetFieldNm() + " is not string.");
		SetFieldStr(Bf, BfL, FieldSerialDesc, JsonVal->GetStr());
		break;
	case oftBool:
		QmAssertR(JsonVal->IsBool(), "Provided JSon data field " + FieldDesc.GetFieldNm() + " is not boolean.");
		SetFieldBool(Bf, BfL, FieldSerialDesc, JsonVal->GetBool());
		break;
	case oftFlt:
		QmAssertR(JsonVal->IsNum(), "Provided JSon data field " + FieldDesc.GetFieldNm() + " is not numeric.");
		SetFieldFlt(Bf, BfL, FieldSerialDesc, JsonVal->GetNum());
		break;
	case oftFltPr: {
		// make sure it's array of length two
		QmAssertR(JsonVal->IsArr(), "Provided JSon data field " + FieldDesc.GetFieldNm() + " is not array.");
		QmAssertR(JsonVal->GetArrVals() == 2, "Provided JSon data field " + FieldDesc.GetFieldNm() + " is not array - expected 2 fields.");
		PJsonVal JsonVal1 = JsonVal->GetArrVal(0);
		PJsonVal JsonVal2 = JsonVal->GetArrVal(1);
		// make sure both elements are numeric
		QmAssertR(JsonVal1->IsNum(), "The first element in the JSon array in data field " + FieldDesc.GetFieldNm() + " is not numeric.");
		QmAssertR(JsonVal2->IsNum(), "The second element in the JSon array in data field " + FieldDesc.GetFieldNm() + " is not numeric.");
		// update
		SetFieldFltPr(Bf, BfL, FieldSerialDesc, TFltPr(JsonVal1->GetNum(), JsonVal2->GetNum()));
		break;
	}
	case oftTm: {
		QmAssertR(JsonVal->IsStr(), "Provided JSon data field " + FieldDesc.GetFieldNm() + " is not string that represents DateTime.");
		TStr TmStr = JsonVal->GetStr();
		TTm Tm = TTm::GetTmFromWebLogDateTimeStr(TmStr, '-', ':', '.', 'T');
		SetFieldTm(Bf, BfL, FieldSerialDesc, Tm);
		break;
	}
	default:
		throw TQmExcept::New("Unsupported JSon data type for DB storage (fixed part): " + FieldDesc.GetFieldTypeStr());
	}
}

/////////////////////

/// Fixed-length field setter
void TRecSerializator::SetFieldInt(char* Bf, const int& BfL, const int& FieldId, const int& Int) {
	SetFieldInt(Bf, BfL, GetFieldSerialDesc(FieldId), Int);
}
/// Fixed-length field setter
void TRecSerializator::SetFieldUInt64(char* Bf, const int& BfL, const int& FieldId, const uint64& UInt64) {
	SetFieldUInt64(Bf, BfL, GetFieldSerialDesc(FieldId), UInt64);
}
/// Fixed-length field setter
void TRecSerializator::SetFieldStr(char* Bf, const int& BfL, const int& FieldId, const TStr& Str) {
	SetFieldStr(Bf, BfL, GetFieldSerialDesc(FieldId), Str);
}
/// Fixed-length field setter
void TRecSerializator::SetFieldBool(char* Bf, const int& BfL, const int& FieldId, const bool& Bool) {
	SetFieldBool(Bf, BfL, GetFieldSerialDesc(FieldId), Bool);
}
/// Fixed-length field setter
void TRecSerializator::SetFieldFlt(char* Bf, const int& BfL, const int& FieldId, const double& Flt) {
	SetFieldFlt(Bf, BfL, GetFieldSerialDesc(FieldId), Flt);
}
/// Fixed-length field setter
void TRecSerializator::SetFieldFltPr(char* Bf, const int& BfL, const int& FieldId, const TFltPr& FltPr) {
	SetFieldFltPr(Bf, BfL, GetFieldSerialDesc(FieldId), FltPr);
}
/// Fixed-length field setter
void TRecSerializator::SetFieldTm(char* Bf, const int& BfL, const int& FieldId, const TTm& Tm) {
	SetFieldTm(Bf, BfL, GetFieldSerialDesc(FieldId), Tm);
}
/// Fixed-length field setter
void TRecSerializator::SetFieldTmMSecs(char* Bf, const int& BfL, const int& FieldId, const uint64& TmMSecs) {
	SetFieldTmMSecs(Bf, BfL, GetFieldSerialDesc(FieldId), TmMSecs);
}


/////////////////////

/// Check this temporary buffer if it must be TOAST-ed
void TRecSerializator::CheckToast(TMOut& SOut, const int& Offset) {
	if (!UseToast) return;
	if (SOut.Len() <= MxToastLen) return;

	// ok, perform TOAST-ing	
	// Store serialized data into store
	TMemBase mb(SOut.GetBfAddr() + Offset + 1, SOut.Len() - Offset - 1, false);
	TPgBlobPt Pt = Store->ToastVal(mb);
	// rewind SOut back to Offset
	SOut.Seek(Offset);
	SOut.PutCh(ToastYes);
	// Serialize
	SOut.AppendBf(&Pt, sizeof(TPgBlobPt));
}

/// Check if given field value is currently TOAST-ed and delete it
void TRecSerializator::CheckToastDel(const TMemBase& InRecMem, const TFieldSerialDesc& FieldSerialDesc) {
	if (UseToast && !FieldSerialDesc.FixedPartP) {
		if (!IsFieldNull(InRecMem, FieldSerialDesc.FieldId)) {
			char* Bf = GetLocationVar(InRecMem, FieldSerialDesc);
			char c = *Bf;
			if (c == ToastYes) {
				Bf++;
				TPgBlobPt Pt;
				Pt = *((TPgBlobPt*)Bf);
				Store->DelToastVal(Pt);
			}
		}
	}
}

void TRecSerializator::SetFieldIntV(TMem& RecMem, TMOut& SOut,
		const TFieldSerialDesc& FieldSerialDesc, const TIntV& IntV) {

	// location of the new variable-length value is at the end of current output stream
	int VarContentOffset = SOut.Len();
	// update it's location in the variable-index
	SetLocationVar(RecMem, FieldSerialDesc, VarContentOffset);
	// update value
	if (UseToast) { SOut.PutCh(ToastNo); }
	IntV.Save(SOut);
	// Perform TOAST-ing if needed
	CheckToast(SOut, VarContentOffset);
}

void TRecSerializator::SetFieldStr(TMem& RecMem, TMOut& SOut,
		const TFieldSerialDesc& FieldSerialDesc, const TStr& Str) {

	// location of the new variable-length value is at the end of current output stream
	int VarContentOffset = SOut.Len();
	// update it's location in the variable-index
	SetLocationVar(RecMem, FieldSerialDesc, VarContentOffset);
	// update value
	if (UseToast) { SOut.PutCh(ToastNo); }
	Str.Save(SOut, FieldSerialDesc.SmallStringP);
	// Perform TOAST-ing if needed
	CheckToast(SOut, VarContentOffset);
}

void TRecSerializator::SetFieldStrV(TMem& RecMem, TMOut& SOut,
		const TFieldSerialDesc& FieldSerialDesc, const TStrV& StrV) {

	// location of the new variable-length value is at the end of current output stream
	int VarContentOffset = SOut.Len();
	// update it's location in the variable-index
	SetLocationVar(RecMem, FieldSerialDesc, VarContentOffset);
	// update value
	if (UseToast) { SOut.PutCh(ToastNo); }
	StrV.Save(SOut);
	// Perform TOAST-ing if needed
	CheckToast(SOut, VarContentOffset);
}

void TRecSerializator::SetFieldFltV(TMem& RecMem, TMOut& SOut,
		const TFieldSerialDesc& FieldSerialDesc, const TFltV& FltV) {

	// location of the new variable-length value is at the end of current output stream
	int VarContentOffset = SOut.Len();
	// update it's location in the variable-index
	SetLocationVar(RecMem, FieldSerialDesc, VarContentOffset);
	// update value
	if (UseToast) { SOut.PutCh(ToastNo); }
	FltV.Save(SOut);
	// Perform TOAST-ing if needed
	CheckToast(SOut, VarContentOffset);
}

void TRecSerializator::SetFieldNumSpV(TMem& RecMem, TMOut& SOut,
		const TFieldSerialDesc& FieldSerialDesc, const TIntFltKdV& SpV) {

	// location of the new variable-length value is at the end of current output stream
	int VarContentOffset = SOut.Len();
	// update it's location in the variable-index
	SetLocationVar(RecMem, FieldSerialDesc, VarContentOffset);
	// update value
	if (UseToast) { SOut.PutCh(ToastNo); }
	SpV.Save(SOut);
	// Perform TOAST-ing if needed
	CheckToast(SOut, VarContentOffset);
}

void TRecSerializator::SetFieldBowSpV(TMem& RecMem, TMOut& SOut,
		const TFieldSerialDesc& FieldSerialDesc, const PBowSpV& SpV) {

	// location of the new variable-length value is at the end of current output stream
	int VarContentOffset = SOut.Len();
	// update it's location in the variable-index
	SetLocationVar(RecMem, FieldSerialDesc, VarContentOffset);
	// update value
	if (UseToast) { SOut.PutCh(ToastNo); }
	SpV->Save(SOut);
	// Perform TOAST-ing if needed
	CheckToast(SOut, VarContentOffset);
}

void TRecSerializator::SetVarJsonVal(TMem& RecMem, TMOut& SOut,
		const TFieldSerialDesc& FieldSerialDesc, const TFieldDesc& FieldDesc,
		const PJsonVal& JsonVal) {

	// call type-appropriate setter
	switch (FieldDesc.GetFieldType()) {
		case oftIntV: {
			QmAssertR(JsonVal->IsArr(), "Provided JSon data field " + FieldDesc.GetFieldNm() + " is not array.");
			TIntV IntV; JsonVal->GetArrIntV(IntV);
			SetFieldIntV(RecMem, SOut, FieldSerialDesc, IntV);
			break;
		}
		case oftStr: {
			QmAssertR(JsonVal->IsStr(), "Provided JSon data field " + FieldDesc.GetFieldNm() + " is not string.");
			TStr Str = JsonVal->GetStr();
			SetFieldStr(RecMem, SOut, FieldSerialDesc, Str);
			break;
		}
		case oftStrV: {
			QmAssertR(JsonVal->IsArr(), "Provided JSon data field " + FieldDesc.GetFieldNm() + " is not array.");
			TStrV StrV; JsonVal->GetArrStrV(StrV);
			SetFieldStrV(RecMem, SOut, FieldSerialDesc, StrV);
			break;
		}
		case oftFltV: {
			QmAssertR(JsonVal->IsArr(), "Provided JSon data field " + FieldDesc.GetFieldNm() + " is not array.");
			TFltV FltV; JsonVal->GetArrNumV(FltV);
			SetFieldFltV(RecMem, SOut, FieldSerialDesc, FltV);
			break;
		}
		case oftBowSpV:
			throw TQmExcept::New("Parsing of BowSpV from JSon not yet implemented");
		case oftNumSpV: {
			QmAssertR(JsonVal->IsArr(), "Provided JSon data field " + FieldDesc.GetFieldNm() + " is not array.");
			TIntFltKdV NumSpV; JsonVal->GetArrNumSpV(NumSpV);
			SetFieldNumSpV(RecMem, SOut, FieldSerialDesc, NumSpV);
			break;
		}
		default:
			throw TQmExcept::New("Unsupported JSon data type for DB storage (variable part) - " + FieldDesc.GetFieldTypeStr());
	}
}

void TRecSerializator::CopyFieldVar(const TMemBase& InRecMem, TMem& FixedMem,
		TMOut& VarSOut, const TFieldSerialDesc& FieldSerialDesc) {

	// make sure we are copying variable field
	QmAssert(!FieldSerialDesc.FixedPartP);
	// set new offset location
	int VarContentOffset = VarSOut.Len();
	SetLocationVar(FixedMem, FieldSerialDesc, VarContentOffset);
	// just copy other variable fields
	int OldVarLength = GetVarPartBfLen(InRecMem, FieldSerialDesc);
	if (OldVarLength > 0) {
		// get location of old variable
		char* OldVarBf = GetLocationVar(InRecMem, FieldSerialDesc);
		// move it to output stream for new serialization
		VarSOut.AppendBf(OldVarBf, OldVarLength);
	}
}

void TRecSerializator::ExtractFixedMem(const TMemBase& InRecMem, TMem& FixedMem) {
	// Reserve fixed space - null map, fixed fields and var-field indexes
	FixedMem.Reserve(VarContentPartOffset);
	// copy fixed part
	Assert(FixedMem.Len() <= InRecMem.Len());
	FixedMem.AddBf(InRecMem.GetBf(), VarContentPartOffset);
}

void TRecSerializator::Merge(const TMem& FixedMem, const TMOut& VarSOut, TMem& OutRecMem) {
	OutRecMem.Reserve(VarContentPartOffset + VarSOut.Len());
	OutRecMem.AddBf(FixedMem.GetBf(), VarContentPartOffset);
	OutRecMem.AddBf(VarSOut.GetBfAddr(), VarSOut.Len());
}

TRecSerializator::TRecSerializator(const TWPt<TStore>& _Store,
		const TStoreSchema& StoreSchema, const TStoreLoc& _TargetStorage):
			TargetStorage(_TargetStorage) {
	
	Store = _Store;
	UseToast = Store->CanToast();
	if (UseToast) {
		MxToastLen = Store->GetMaxToastLen();
	}
	const int Fields = Store->GetFields();

	// fixed part starts after null-flags
	FixedPartOffset = (int)ceil((float)Fields / 8);
	// variable part starts same place before any fixed-width fields identified
	VarIndexPartOffset = FixedPartOffset;

	// maintaining current offsets and counts
	int FixedIndexOffset = 0;
	int VarIndexOffset = 0;
	int VarFieldCount = 0;

	for (int FieldId = 0; FieldId < Fields; FieldId++) {
		QmAssert(Store->IsFieldId(FieldId));
		// get field name
		const TStr& FieldName = Store->GetFieldNm(FieldId);
		// get field description
		const TFieldDesc& FieldDesc = Store->GetFieldDesc(FieldId);
		// get extended field description from schema
		const TFieldDescEx& FieldDescEx = StoreSchema.FieldExH.GetDat(FieldName);
		// skip field if it does not match targeted storage
		if (FieldDescEx.FieldStoreLoc != TargetStorage) { continue; }
		// check if field is fixed-width and if yes, what is its width
		int FixedSize = 0; bool FixedP = true;
		switch (FieldDesc.GetFieldType()) {
			case oftInt: FixedSize = sizeof(int); break;
			case oftIntV: FixedP = false; break;
			case oftUInt64: FixedSize = sizeof(uint64); break;
			case oftStr: FixedP = FieldDescEx.CodebookP; if (FixedP) { FixedSize = sizeof(int); } break;
			case oftStrV: FixedP = false; break;
			case oftBool: FixedSize = sizeof(bool); break;
			case oftFlt: FixedSize = sizeof(double); break;
			case oftFltPr: FixedSize = sizeof(double) * 2; break;
			case oftFltV: FixedP = false; break;
			case oftTm: FixedSize = sizeof(uint64); break;
			case oftNumSpV: FixedP = false; break;
			case oftBowSpV: FixedP = false; break;
			default: throw TQmExcept::New("Unknown field type " + FieldDesc.GetFieldTypeStr());
		}
		// move variable offset for the fixed size of current field
		VarIndexPartOffset += FixedSize;
		// prepare field serialization description
		TFieldSerialDesc FieldSerialDesc;
		FieldSerialDesc.FieldId = FieldDesc.GetFieldId();
		FieldSerialDesc.StoreLoc = FieldDescEx.FieldStoreLoc;
		FieldSerialDesc.NullMapByte = FieldId / 8;
		FieldSerialDesc.NullMapMask = 1u << (FieldId % 8);
		FieldSerialDesc.FixedPartP = FixedP;
		FieldSerialDesc.Offset = (FixedP ? FixedIndexOffset : VarIndexOffset);
		FieldSerialDesc.CodebookP = FieldDescEx.CodebookP;
		FieldSerialDesc.SmallStringP = FieldDescEx.SmallStringP;
		FieldSerialDesc.DefaultVal = FieldDescEx.DefaultVal;
		// remember serialization description
		int FieldSerialDescId = FieldSerialDescV.Add(FieldSerialDesc);
		// remember mapping from field id to serialization description id
		FieldIdToSerialDescIdH.AddDat(FieldSerialDesc.FieldId, FieldSerialDescId);
		// accordingly update fixed or variable-index offsets
		if (FixedP) {
			FixedIndexOffset += FixedSize;
		} else {
			VarIndexOffset += sizeof(int);
			VarFieldCount++;
		}
	}
	// var-index part consists of integers that are offsets for specific field
	VarContentPartOffset = VarIndexPartOffset + VarFieldCount * sizeof(int);
}

void TRecSerializator::Load(TSIn& SIn) {
	TargetStorage = TStoreLoc(TInt(SIn).Val);
	FixedPartOffset.Load(SIn);
	VarIndexPartOffset.Load(SIn);
	VarContentPartOffset.Load(SIn);
	FieldSerialDescV.Load(SIn);
	FieldIdToSerialDescIdH.Load(SIn);
	CodebookH.Load(SIn);
	UseToast.Load(SIn);
	MxToastLen.Load(SIn);
}

void TRecSerializator::Save(TSOut& SOut) {
	TInt(TargetStorage).Save(SOut);
	FixedPartOffset.Save(SOut);
	VarIndexPartOffset.Save(SOut);
	VarContentPartOffset.Save(SOut);
	FieldSerialDescV.Save(SOut);
	FieldIdToSerialDescIdH.Save(SOut);
	CodebookH.Save(SOut);
	UseToast.Save(SOut);
	MxToastLen.Save(SOut);
}

void TRecSerializator::Serialize(const PJsonVal& RecVal, TMem& RecMem, const TWPt<TStore>& Store) {
	// Reserve fixed space - null map, fixed fields and var-field indexes
	TMem FixedMem(VarContentPartOffset);
	// Overwrite fixed part with zeros to start with
	FixedMem.GenZeros(VarContentPartOffset);
	// Prepare output stream for storing variable width values
	TMOut VarSOut;

	// iterate over fields and serialize them
	for (int FieldSerialDescId = 0; FieldSerialDescId < FieldSerialDescV.Len(); FieldSerialDescId++) {
		const TFieldSerialDesc& FieldSerialDesc = FieldSerialDescV[FieldSerialDescId];
		// get field description
		const TFieldDesc& FieldDesc = Store->GetFieldDesc(FieldSerialDesc.FieldId);
		TStr FieldName = FieldDesc.GetFieldNm();
		// parse field value from provided JSon
		PJsonVal FieldVal;
		// figure out value when not provided directly
		if (!RecVal->IsObjKey(FieldName)){
			// check if the field is a surrogate for a field join
			if (!FieldSerialDesc.DefaultVal.Empty()) {
				// use the provided default value
				FieldVal = FieldSerialDesc.DefaultVal;
			} else if (FieldDesc.IsNullable()) {
				// value not provided and object is nullable, so we set it to NULL
				SetFieldNull(FixedMem, FieldSerialDesc, true);
				// update variable-length index to point to the end of stream
				if (!FieldSerialDesc.FixedPartP) {
					SetLocationVar(FixedMem, FieldSerialDesc, VarSOut.Len());
				}
				// we are done with this field
				continue;
			} else {
				// report missing field value since no other option available
				TStr Str = TJsonVal::GetStrFromVal(RecVal);
				throw TQmExcept::New("JSon data is missing field - expecting " + FieldName);
			}
		}
		// load field value when default not already loaded
		if (FieldVal.Empty()) {
			FieldVal = RecVal->GetObjKey(FieldName);
		}
		// set the field as specified
		if (FieldVal->IsNull()) {
			// we are setting field explicitly to null
			QmAssertR(FieldDesc.IsNullable(), "Non-nullable field " + FieldName + " set to null");
			SetFieldNull(FixedMem, FieldSerialDesc, true);
			// if not from fixed part, point variable-length index to the end of stream
			if (!FieldSerialDesc.FixedPartP) {
				SetLocationVar(FixedMem, FieldSerialDesc, VarSOut.Len());
			}            
		} else if (FieldSerialDesc.FixedPartP) {
			SetFixedJsonVal(FixedMem, FieldSerialDesc, FieldDesc, FieldVal);
		} else {
			SetVarJsonVal(FixedMem, VarSOut, FieldSerialDesc, FieldDesc, FieldVal);
		}
	}

	// merge fixed and variable parts for final result
	Merge(FixedMem, VarSOut, RecMem);
}

void TRecSerializator::SerializeUpdateInPlace(const PJsonVal& RecVal, 
	TThinMIn MIn, const TWPt<TStore>& Store, TIntSet& ChangedFieldIdSet) {

	// iterate over fields and serialize them
	for (int FieldSerialDescId = 0; FieldSerialDescId < FieldSerialDescV.Len(); FieldSerialDescId++) {
		const TFieldSerialDesc& FieldSerialDesc = FieldSerialDescV[FieldSerialDescId];
		// get field description
		const TFieldDesc& FieldDesc = Store->GetFieldDesc(FieldSerialDesc.FieldId);
		TStr FieldName = FieldDesc.GetFieldNm();
		if (RecVal->IsObjKey(FieldName)) {
			// new value, must update
			int BfL = MIn.Len();
			char* Bf = MIn.GetBfAddrChar();
			PJsonVal JsonVal = RecVal->GetObjKey(FieldName);
			if (JsonVal->IsNull()) {
				// we are setting field explicitly to null
				QmAssertR(FieldDesc.IsNullable(), "Non-nullable field " + FieldName + " set to null");
				SetFieldNull(Bf, BfL, FieldSerialDesc.FieldId, true);
			} else {
				// remove null flag
				SetFieldNull(Bf, BfL, FieldSerialDesc.FieldId, true);
				// serialize the field
				QmAssert(FieldSerialDesc.FixedPartP);
				SetFixedJsonVal((char*)Bf, BfL, FieldSerialDesc, FieldDesc, JsonVal);				
			}
			// remember for reporting back that we updated the field
			ChangedFieldIdSet.AddKey(FieldDesc.GetFieldId());
		}
	}
}

void TRecSerializator::SerializeUpdate(const PJsonVal& RecVal, const TMemBase& InRecMem,
		TMem& OutRecMem, const TWPt<TStore>& Store, TIntSet& ChangedFieldIdSet) {

	// split to fixed and variable parts
	TMem FixedMem; TMOut VarSOut; ExtractFixedMem(InRecMem, FixedMem);

	// iterate over fields and serialize them
	for (int FieldSerialDescId = 0; FieldSerialDescId < FieldSerialDescV.Len(); FieldSerialDescId++) {
		const TFieldSerialDesc& FieldSerialDesc = FieldSerialDescV[FieldSerialDescId];
		// get field description
		const TFieldDesc& FieldDesc = Store->GetFieldDesc(FieldSerialDesc.FieldId);
		TStr FieldName = FieldDesc.GetFieldNm();
		// figure out value when not provided directly
		if (!RecVal->IsObjKey(FieldName)){
			// copy the variable field when no update to it is provided
			// fixed length variables are already copied on start
			if (!FieldSerialDesc.FixedPartP) {
				CopyFieldVar(InRecMem, FixedMem, VarSOut, FieldSerialDesc);
			}
		} else {
			// new value, must update
			//if (UseToast && !FieldSerialDesc.FixedPartP) {
			//	if (!IsFieldNull(InRecMem, FieldSerialDesc.FieldId)) {
			//		// remove toast
			//		char* Bf = GetLocationVar(InRecMem, FieldSerialDesc);
			//		char c = *Bf;
			//		if (c == ToastYes) {
			//			Bf++;
			//			TPgBlobPt Pt;
			//			Pt = *((TPgBlobPt*)Bf);
			//			Store->DelToastVal(Pt);
			//		}
			//	}
			//}
			PJsonVal JsonVal = RecVal->GetObjKey(FieldName);
			if (JsonVal->IsNull()) {
				// we are setting field explicitly to null
				QmAssertR(FieldDesc.IsNullable(), "Non-nullable field " + FieldName + " set to null");
				SetFieldNull(FixedMem, FieldSerialDesc, true);
				// if not from fixed part, point variable-length index to the end of stream
				if (!FieldSerialDesc.FixedPartP) {
					SetLocationVar(FixedMem, FieldSerialDesc, VarSOut.Len());
				}            
			} else {
				// remove null flag
				SetFieldNull(FixedMem, FieldSerialDesc, false);
				// serialize the field
				if (FieldSerialDesc.FixedPartP) {
					SetFixedJsonVal(FixedMem, FieldSerialDesc, FieldDesc, JsonVal);
				} else {
					SetVarJsonVal(FixedMem, VarSOut, FieldSerialDesc, FieldDesc, JsonVal);
				}
			}
			// remember for reporting back that we updated the field
			ChangedFieldIdSet.AddKey(FieldDesc.GetFieldId());
		}
	}

	// merge fixed and variable parts for final result
	Merge(FixedMem, VarSOut, OutRecMem);
}

///////////////////////////
/// Field getter
bool TRecSerializator::IsFieldNull(TThinMIn& min, const int& FieldId) const {
	const TFieldSerialDesc& FieldSerialDesc = GetFieldSerialDesc(FieldId);
	const char* bf = min.GetBfAddrChar() + FieldSerialDesc.NullMapByte;
	return ((*bf & FieldSerialDesc.NullMapMask) != 0);
}
/// Field getter
int TRecSerializator::GetFieldInt(TThinMIn& min, const int& FieldId) const {
	const char* bf = GetLocationFixed(min, GetFieldSerialDesc(FieldId));
	return *((int*)bf);
}
/// Field getter
void TRecSerializator::GetFieldIntV(TThinMIn& min, const int& FieldId, TIntV& IntV) const {
	min.MoveTo(GetOffsetVar(min, GetFieldSerialDesc(FieldId)));
	if (UseToast && min.GetCh() == ToastYes) {
		TPgBlobPt Pt;
		min.GetBf(&Pt, sizeof(TPgBlobPt));
		TMem Mem;
		Store->UnToastVal(Pt, Mem);
		TThinMIn min2(Mem);
		IntV.Load(min2);
	} else {
		IntV.Load(min);
	}
}
/// Field getter
uint64 TRecSerializator::GetFieldUInt64(TThinMIn& min, const int& FieldId) const {
	const char* bf = GetLocationFixed(min, GetFieldSerialDesc(FieldId));
	return *((uint64*)bf);
}
/// Field getter
TStr TRecSerializator::GetFieldStr(TThinMIn& min, const int& FieldId) const {
	const TFieldSerialDesc& FieldSerialDesc = GetFieldSerialDesc(FieldId);
	if (FieldSerialDesc.FixedPartP) {
		// get pointer to location
		char* bf = GetLocationFixed(min, FieldSerialDesc);
		// cast to codebook id value
		int StrId = *((int*)bf);
		// return string from codebook
		return CodebookH.GetKey(StrId);
	} else {
		min.MoveTo(GetOffsetVar(min, GetFieldSerialDesc(FieldId)));
		if (UseToast && min.GetCh() == ToastYes) {
			TPgBlobPt Pt;
			min.GetBf(&Pt, sizeof(TPgBlobPt));
			TMem Mem;
			Store->UnToastVal(Pt, Mem);
			TThinMIn min2(Mem);
			TStr Str;
			Str.Load(min2, FieldSerialDesc.SmallStringP);
			return Str;
		} else {
			TStr Str;
			Str.Load(min, FieldSerialDesc.SmallStringP);
			return Str;
		}
		//TStr Str; 
		//Str.Load(min, FieldSerialDesc.SmallStringP);
		//return Str;
	}
}
/// Field getter
void TRecSerializator::GetFieldStrV(TThinMIn& min, const int& FieldId, TStrV& StrV) const {
	min.MoveTo(GetOffsetVar(min, GetFieldSerialDesc(FieldId)));
	if (UseToast && min.GetCh() == ToastYes) {
		TPgBlobPt Pt;
		min.GetBf(&Pt, sizeof(TPgBlobPt));
		TMem Mem;
		Store->UnToastVal(Pt, Mem);
		TThinMIn min2(Mem);
		StrV.Load(min2);
	} else {
		StrV.Load(min);
	}
	//StrV.Load(min);
}
/// Field getter
bool TRecSerializator::GetFieldBool(TThinMIn& min, const int& FieldId) const {
	const char* bf = GetLocationFixed(min, GetFieldSerialDesc(FieldId));
	return *((bool*)bf);
}
/// Field getter
double TRecSerializator::GetFieldFlt(TThinMIn& min, const int& FieldId) const {
	const char* bf = GetLocationFixed(min, GetFieldSerialDesc(FieldId));
	return *((double*)bf);
}
/// Field getter
TFltPr TRecSerializator::GetFieldFltPr(TThinMIn& min, const int& FieldId) const {
	const char* bf = GetLocationFixed(min, GetFieldSerialDesc(FieldId));
	return TFltPr(*((double*)bf), *(((double*)bf) + 1));
}
/// Field getter
void TRecSerializator::GetFieldFltV(TThinMIn& min, const int& FieldId, TFltV& FltV) const {
	min.MoveTo(GetOffsetVar(min, GetFieldSerialDesc(FieldId)));
	if (UseToast && min.GetCh() == ToastYes) {
		TPgBlobPt Pt;
		min.GetBf(&Pt, sizeof(TPgBlobPt));
		TMem Mem;
		Store->UnToastVal(Pt, Mem);
		TThinMIn min2(Mem);
		FltV.Load(min2);
	} else {
		FltV.Load(min);
	}
	//FltV.Load(min);
}
/// Field getter
void TRecSerializator::GetFieldTm(TThinMIn& min, const int& FieldId, TTm& Tm) const {
	Tm = TTm::GetTmFromMSecs(GetFieldTmMSecs(min, FieldId));
}
/// Field getter
uint64 TRecSerializator::GetFieldTmMSecs(TThinMIn& min, const int& FieldId) const {
	const char* bf = GetLocationFixed(min, GetFieldSerialDesc(FieldId));
	return *((uint64*)bf);
}
/// Field getter
void TRecSerializator::GetFieldNumSpV(TThinMIn& min, const int& FieldId, TIntFltKdV& SpV) const {
	min.MoveTo(GetOffsetVar(min, GetFieldSerialDesc(FieldId)));
	if (UseToast && min.GetCh() == ToastYes) {
		TPgBlobPt Pt;
		min.GetBf(&Pt, sizeof(TPgBlobPt));
		TMem Mem;
		Store->UnToastVal(Pt, Mem);
		TThinMIn min2(Mem);
		SpV.Load(min2);
	} else {
		SpV.Load(min);
	}
	//SpV.Load(min);
}
/// Field getter
void TRecSerializator::GetFieldBowSpV(TThinMIn& min, const int& FieldId, PBowSpV& SpV) const {
	min.MoveTo(GetOffsetVar(min, GetFieldSerialDesc(FieldId)));
	if (UseToast && min.GetCh() == ToastYes) {
		TPgBlobPt Pt;
		min.GetBf(&Pt, sizeof(TPgBlobPt));
		TMem Mem;
		Store->UnToastVal(Pt, Mem);
		TThinMIn min2(Mem);
		TBowSpV::Load(min2);
	} else {
		TBowSpV::Load(min);
	}
	//SpV = TBowSpV::Load(min);
}
///////////////////////////

bool TRecSerializator::IsFieldNull(const TMemBase& RecMem, const int& FieldId) const {
	//const TFieldSerialDesc& FieldSerialDesc = GetFieldSerialDesc(FieldId);
	//char* bf = RecMem.GetBf() + FieldSerialDesc.NullMapByte;
	//return ((*bf & FieldSerialDesc.NullMapMask) != 0);
	return IsFieldNull(TThinMIn(RecMem), FieldId);
}

int TRecSerializator::GetFieldInt(const TMemBase& RecMem, const int& FieldId) const {
	//// get pointer to location
	//char* bf = GetLocationFixed(RecMem, GetFieldSerialDesc(FieldId));
	//// cast to return value
	//return *((int*)bf);
	return GetFieldInt(TThinMIn(RecMem), FieldId);
}

void TRecSerializator::GetFieldIntV(const TMemBase& RecMem, const int& FieldId, TIntV& IntV) const {
	//// prepare input stream and move to the variable location
	//TThinMIn MIn(RecMem);
	//MIn.MoveTo(GetOffsetVar(RecMem, GetFieldSerialDesc(FieldId)));
	//// load return value from stream
	//IntV.Load(MIn);
	GetFieldIntV(TThinMIn(RecMem), FieldId, IntV);
}

uint64 TRecSerializator::GetFieldUInt64(const TMemBase& RecMem, const int& FieldId) const {
	//// get pointer to location
	//char* bf = GetLocationFixed(RecMem, GetFieldSerialDesc(FieldId));
	//// cast to return value
	//return *((uint64*)bf);
	return GetFieldUInt64(TThinMIn(RecMem), FieldId);
}

TStr TRecSerializator::GetFieldStr(const TMemBase& RecMem, const int& FieldId) const {
	//const TFieldSerialDesc& FieldSerialDesc = GetFieldSerialDesc(FieldId);
	//if (FieldSerialDesc.FixedPartP) {
	//	// get pointer to location
	//	char* bf = GetLocationFixed(RecMem, FieldSerialDesc);
	//	// cast to codebook id value
	//	int StrId = *((int*)bf);
	//	// return string from codebook
	//	return CodebookH.GetKey(StrId);
	//} else {
	//	// prepare input stream and move to the variable location
	//	TThinMIn MIn(RecMem);
	//	MIn.MoveTo(GetOffsetVar(RecMem, FieldSerialDesc));
	//	// load return value from stream
	//	TStr Str; Str.Load(MIn, FieldSerialDesc.SmallStringP);
	//	return Str;
	//}
	return GetFieldStr(TThinMIn(RecMem), FieldId);
}

void TRecSerializator::GetFieldStrV(const TMemBase& RecMem, const int& FieldId, TStrV& StrV) const {
	//// prepare input stream and move to the variable location
	//TThinMIn MIn(RecMem);
	//MIn.MoveTo(GetOffsetVar(RecMem, GetFieldSerialDesc(FieldId)));
	//// load return value from stream
	//StrV.Load(MIn);
	GetFieldStrV(TThinMIn(RecMem), FieldId, StrV);
}

bool TRecSerializator::GetFieldBool(const TMemBase& RecMem, const int& FieldId) const {
	//// get pointer to location
	//char* bf = GetLocationFixed(RecMem, GetFieldSerialDesc(FieldId));
	//// cast to return value
	//return *((bool*)bf);
	return GetFieldBool(TThinMIn(RecMem), FieldId);
}

double TRecSerializator::GetFieldFlt(const TMemBase& RecMem, const int& FieldId) const {
	//// get pointer to location
	//char* bf = GetLocationFixed(RecMem, GetFieldSerialDesc(FieldId));
	//// cast to return value
	//return *((double*)bf);
	return GetFieldFlt(TThinMIn(RecMem), FieldId);
}

TFltPr TRecSerializator::GetFieldFltPr(const TMemBase& RecMem, const int& FieldId) const {
	//// get pointer to location
	//char* bf = GetLocationFixed(RecMem, GetFieldSerialDesc(FieldId));
	//// cast to return value
	//return TFltPr(*((double*)bf), *(((double*)bf) + 1));
	return GetFieldFltPr(TThinMIn(RecMem), FieldId);
}

void TRecSerializator::GetFieldFltV(const TMemBase& RecMem, const int& FieldId, TFltV& FltV) const {
	//// prepare input stream and move to the variable location
	//TThinMIn MIn(RecMem);
	//MIn.MoveTo(GetOffsetVar(RecMem, GetFieldSerialDesc(FieldId)));
	//// load return value from stream
	//FltV.Load(MIn);
	GetFieldFltV(TThinMIn(RecMem), FieldId, FltV);
}

void TRecSerializator::GetFieldTm(const TMemBase& RecMem, const int& FieldId, TTm& Tm) const {
	//// get pointer to location
	//char* bf = GetLocationFixed(RecMem, GetFieldSerialDesc(FieldId));
	//// cast to return value
	//uint64 val = *((uint64*)bf);
	//Tm = TTm::GetTmFromMSecs(val);
	GetFieldTm(TThinMIn(RecMem), FieldId, Tm);
}

uint64 TRecSerializator::GetFieldTmMSecs(const TMemBase& RecMem, const int& FieldId) const {
	//// get pointer to location
	//char* bf = GetLocationFixed(RecMem, GetFieldSerialDesc(FieldId));
	//// cast to return value
	//return *((uint64*)bf);
	return GetFieldTmMSecs(TThinMIn(RecMem), FieldId);
}

void TRecSerializator::GetFieldNumSpV(const TMemBase& RecMem, const int& FieldId, TIntFltKdV& SpV) const {
	//// prepare input stream and move to the variable location
	//TThinMIn MIn(RecMem);
	//MIn.MoveTo(GetOffsetVar(RecMem, GetFieldSerialDesc(FieldId)));
	//// load return value from stream
	//SpV.Load(MIn);
	GetFieldNumSpV(TThinMIn(RecMem), FieldId, SpV);
}

void TRecSerializator::GetFieldBowSpV(const TMemBase& RecMem, const int& FieldId, PBowSpV& SpV) const {
	//// prepare input stream and move to the variable location
	//TThinMIn MIn(RecMem);
	//MIn.MoveTo(GetOffsetVar(RecMem, GetFieldSerialDesc(FieldId)));
	//// load return value from stream
	//SpV = TBowSpV::Load(MIn);
	GetFieldBowSpV(TThinMIn(RecMem), FieldId, SpV);
}

void TRecSerializator::SetFieldNull(const TMemBase& InRecMem, TMem& OutRecMem, const int& FieldId) {
	// different handling for fixed and variable fields
	const TFieldSerialDesc& FieldSerialDesc = GetFieldSerialDesc(FieldId);
	if (FieldSerialDesc.FixedPartP) {
		// copy existing serialization
		OutRecMem.Copy(InRecMem);
		// just mark fixed field as null
		SetFieldNull(OutRecMem, FieldSerialDesc, true);
	} else {
		// split to fixed and variable parts
		TMem FixedMem; TMOut VarSOut; ExtractFixedMem(InRecMem, FixedMem);
		// iterate over fields and serialize them
		for (int FieldSerialDescId = 0; FieldSerialDescId < FieldSerialDescV.Len(); FieldSerialDescId++) {
			const TFieldSerialDesc& FieldSerialDesc = FieldSerialDescV[FieldSerialDescId];
			if (FieldSerialDesc.FieldId == FieldId) {
				// check if value is toasted => we need to delete it
				CheckToastDel(InRecMem, FieldSerialDesc);
				// this is the field we are setting to NULL
				SetFieldNull(FixedMem, FieldSerialDesc, true);
				// update variable-length index to point
				QmAssert(!FieldSerialDesc.FixedPartP);
				SetLocationVar(FixedMem, FieldSerialDesc, VarSOut.Len());
			} else if (!FieldSerialDesc.FixedPartP) {
				// just copy other variable fields
				CopyFieldVar(InRecMem, FixedMem, VarSOut, FieldSerialDesc);
			}
		}
		// merge fixed and variable parts for final result
		Merge(FixedMem, VarSOut, OutRecMem);
	}
}

void TRecSerializator::SetFieldInt(const TMemBase& InRecMem,
		TMem& OutRecMem, const int& FieldId, const int& Int) {

	const TFieldSerialDesc& FieldSerialDesc = GetFieldSerialDesc(FieldId);
	// copy existing serialization
	OutRecMem.Copy(InRecMem);
	// remove null flag, just in case
	SetFieldNull(OutRecMem, FieldSerialDesc, false);
	// update the value
	SetFieldInt(OutRecMem, FieldSerialDesc, Int);
}

void TRecSerializator::SetFieldIntV(const TMemBase& InRecMem, TMem& OutRecMem, const int& FieldId, const TIntV& IntV) {
	// split to fixed and variable parts
	TMem FixedMem; TMOut VarSOut; ExtractFixedMem(InRecMem, FixedMem);
	// iterate over fields and serialize them
	for (int FieldSerialDescId = 0; FieldSerialDescId < FieldSerialDescV.Len(); FieldSerialDescId++) {
		const TFieldSerialDesc& FieldSerialDesc = FieldSerialDescV[FieldSerialDescId];
		if (FieldSerialDesc.FieldId == FieldId) {
			// check if value is toasted => we need to delete it
			CheckToastDel(InRecMem, FieldSerialDesc);
			// remove null flag, just in case
			SetFieldNull(FixedMem, FieldSerialDesc, false);
			// serialize to record buffer
			SetFieldIntV(FixedMem, VarSOut, FieldSerialDesc, IntV);
		} else if (!FieldSerialDesc.FixedPartP) {
			// just copy other variable fields
			CopyFieldVar(InRecMem, FixedMem, VarSOut, FieldSerialDesc);
		}
	}
	// merge fixed and variable parts for final result
	Merge(FixedMem, VarSOut, OutRecMem);
}

void TRecSerializator::SetFieldUInt64(const TMemBase& InRecMem,
		TMem& OutRecMem, const int& FieldId, const uint64& UInt64) {

	const TFieldSerialDesc& FieldSerialDesc = GetFieldSerialDesc(FieldId);
	// copy existing serialization
	OutRecMem.Copy(InRecMem);
	// remove null flag, just in case
	SetFieldNull(OutRecMem, FieldSerialDesc, false);
	// update the value
	SetFieldUInt64(OutRecMem, FieldSerialDesc, UInt64);
}

void TRecSerializator::SetFieldStr(const TMemBase& InRecMem,
		TMem& OutRecMem, const int& FieldId, const TStr& Str) {

	// different handling for codebook strings and normal strings
	const TFieldSerialDesc& FieldSerialDesc = GetFieldSerialDesc(FieldId);
	if (FieldSerialDesc.FixedPartP) {
		// copy existing serialization
		OutRecMem.Copy(InRecMem);
		// remove null flag, just in case
		SetFieldNull(OutRecMem, FieldSerialDesc, false);
		// update value
		SetFieldStr(OutRecMem, FieldSerialDesc, Str);
	} else {
		// split to fixed and variable parts
		TMem FixedMem; TMOut VarSOut; ExtractFixedMem(InRecMem, FixedMem);
		// iterate over fields and serialize them
		for (int FieldSerialDescId = 0; FieldSerialDescId < FieldSerialDescV.Len(); FieldSerialDescId++) {
			const TFieldSerialDesc& FieldSerialDesc = FieldSerialDescV[FieldSerialDescId];
			if (FieldSerialDesc.FieldId == FieldId) {
				// check if value is toasted => we need to delete it
				CheckToastDel(InRecMem, FieldSerialDesc);
				// remove null flag, just in case
				SetFieldNull(FixedMem, FieldSerialDesc, false);
				// serialize to record buffer
				SetFieldStr(FixedMem, VarSOut, FieldSerialDesc, Str);
			} else if (!FieldSerialDesc.FixedPartP) {
				// just copy other variable fields
				CopyFieldVar(InRecMem, FixedMem, VarSOut, FieldSerialDesc);
			}
		}
		// merge fixed and variable parts for final result
		Merge(FixedMem, VarSOut, OutRecMem);
	}
}

void TRecSerializator::SetFieldStrV(const TMemBase& InRecMem,
		TMem& OutRecMem, const int& FieldId, const TStrV& StrV) {

	// split to fixed and variable parts
	TMem FixedMem; TMOut VarSOut; ExtractFixedMem(InRecMem, FixedMem);
	// iterate over fields and serialize them
	for (int FieldSerialDescId = 0; FieldSerialDescId < FieldSerialDescV.Len(); FieldSerialDescId++) {
		const TFieldSerialDesc& FieldSerialDesc = FieldSerialDescV[FieldSerialDescId];
		if (FieldSerialDesc.FieldId == FieldId) {
			// check if value is toasted => we need to delete it
			CheckToastDel(InRecMem, FieldSerialDesc);
			// remove null flag, just in case
			SetFieldNull(FixedMem, FieldSerialDesc, false);
			// serialize to record buffer
			SetFieldStrV(FixedMem, VarSOut, FieldSerialDesc, StrV);
		} else if (!FieldSerialDesc.FixedPartP) {
			// just copy other variable fields
			CopyFieldVar(InRecMem, FixedMem, VarSOut, FieldSerialDesc);
		}
	}
	// merge fixed and variable parts for final result
	Merge(FixedMem, VarSOut, OutRecMem);
}

void TRecSerializator::SetFieldBool(const TMemBase& InRecMem,
		TMem& OutRecMem, const int& FieldId, const bool& Bool) {

	const TFieldSerialDesc& FieldSerialDesc = GetFieldSerialDesc(FieldId);
	// copy existing serialization
	OutRecMem.Copy(InRecMem);
	// remove null flag, just in case
	SetFieldNull(OutRecMem, FieldSerialDesc, false);
	// update the value
	SetFieldBool(OutRecMem, FieldSerialDesc, Bool);
}

void TRecSerializator::SetFieldFlt(const TMemBase& InRecMem,
		TMem& OutRecMem, const int& FieldId, const double& Flt) {

	const TFieldSerialDesc& FieldSerialDesc = GetFieldSerialDesc(FieldId);
	// copy existing serialization
	OutRecMem.Copy(InRecMem);
	// remove null flag, just in case
	SetFieldNull(OutRecMem, FieldSerialDesc, false);
	// update the value
	SetFieldFlt(OutRecMem, FieldSerialDesc, Flt);
}

void TRecSerializator::SetFieldFltPr(const TMemBase& InRecMem,
		TMem& OutRecMem, const int& FieldId, const TFltPr& FltPr) {

	const TFieldSerialDesc& FieldSerialDesc = GetFieldSerialDesc(FieldId);
	// copy existing serialization
	OutRecMem.Copy(InRecMem);
	// remove null flag, just in case
	SetFieldNull(OutRecMem, FieldSerialDesc, false);
	// update the value
	SetFieldFltPr(OutRecMem, FieldSerialDesc, FltPr);
}

void TRecSerializator::SetFieldFltV(const TMemBase& InRecMem,
		TMem& OutRecMem, const int& FieldId, const TFltV& FltV) {

	// split to fixed and variable parts
	TMem FixedMem; TMOut VarSOut; ExtractFixedMem(InRecMem, FixedMem);
	// iterate over fields and serialize them
	for (int FieldSerialDescId = 0; FieldSerialDescId < FieldSerialDescV.Len(); FieldSerialDescId++) {
		const TFieldSerialDesc& FieldSerialDesc = FieldSerialDescV[FieldSerialDescId];
		if (FieldSerialDesc.FieldId == FieldId) {
			// check if value is toasted => we need to delete it
			CheckToastDel(InRecMem, FieldSerialDesc);
			// remove null flag, just in case
			SetFieldNull(FixedMem, FieldSerialDesc, false);
			// serialize to record buffer
			SetFieldFltV(FixedMem, VarSOut, FieldSerialDesc, FltV);
		} else if (!FieldSerialDesc.FixedPartP) {
			// just copy other variable fields
			CopyFieldVar(InRecMem, FixedMem, VarSOut, FieldSerialDesc);
		}
	}
	// merge fixed and variable parts for final result
	Merge(FixedMem, VarSOut, OutRecMem);
}

void TRecSerializator::SetFieldTm(const TMemBase& InRecMem,
		TMem& OutRecMem, const int& FieldId, const TTm& Tm) {

	const TFieldSerialDesc& FieldSerialDesc = GetFieldSerialDesc(FieldId);
	// copy existing serialization
	OutRecMem.Copy(InRecMem);
	// remove null flag, just in case
	SetFieldNull(OutRecMem, FieldSerialDesc, false);
	// update the value
	SetFieldTm(OutRecMem, FieldSerialDesc, Tm);
}

void TRecSerializator::SetFieldTmMSecs(const TMemBase& InRecMem,
		TMem& OutRecMem, const int& FieldId, const uint64& TmMSecs) {

	const TFieldSerialDesc& FieldSerialDesc = GetFieldSerialDesc(FieldId);
	// copy existing serialization
	OutRecMem.Copy(InRecMem);
	// remove null flag, just in case
	SetFieldNull(OutRecMem, FieldSerialDesc, false);
	// update the value
	SetFieldTmMSecs(OutRecMem, FieldSerialDesc, TmMSecs);
}

void TRecSerializator::SetFieldNumSpV(const TMemBase& InRecMem,
		TMem& OutRecMem, const int& FieldId, const TIntFltKdV& SpV) {

	// split to fixed and variable parts
	TMem FixedMem; TMOut VarSOut; ExtractFixedMem(InRecMem, FixedMem);
	// iterate over fields and serialize them
	for (int FieldSerialDescId = 0; FieldSerialDescId < FieldSerialDescV.Len(); FieldSerialDescId++) {
		const TFieldSerialDesc& FieldSerialDesc = FieldSerialDescV[FieldSerialDescId];
		if (FieldSerialDesc.FieldId == FieldId) {
			// check if value is toasted => we need to delete it
			CheckToastDel(InRecMem, FieldSerialDesc);
			// remove null flag, just in case
			SetFieldNull(FixedMem, FieldSerialDesc, false);
			// serialize to record buffer
			SetFieldNumSpV(FixedMem, VarSOut, FieldSerialDesc, SpV);
		} else if (!FieldSerialDesc.FixedPartP) {
			// just copy other variable fields
			CopyFieldVar(InRecMem, FixedMem, VarSOut, FieldSerialDesc);
		}
	}
	// merge fixed and variable parts for final result
	Merge(FixedMem, VarSOut, OutRecMem);
}

void TRecSerializator::SetFieldBowSpV(const TMemBase& InRecMem,
		TMem& OutRecMem, const int& FieldId, const PBowSpV& SpV) {

	// split to fixed and variable parts
	TMem FixedMem; TMOut VarSOut; ExtractFixedMem(InRecMem, FixedMem);
	// iterate over fields and serialize them
	for (int FieldSerialDescId = 0; FieldSerialDescId < FieldSerialDescV.Len(); FieldSerialDescId++) {
		const TFieldSerialDesc& FieldSerialDesc = FieldSerialDescV[FieldSerialDescId];
		if (FieldSerialDesc.FieldId == FieldId) {
			// check if value is toasted => we need to delete it
			CheckToastDel(InRecMem, FieldSerialDesc);
			// remove null flag, just in case
			SetFieldNull(FixedMem, FieldSerialDesc, false);
			// serialize to record buffer
			SetFieldBowSpV(FixedMem, VarSOut, FieldSerialDesc, SpV);
		} else if (!FieldSerialDesc.FixedPartP) {
			// just copy other variable fields
			CopyFieldVar(InRecMem, FixedMem, VarSOut, FieldSerialDesc);
		}
	}
	// merge fixed and variable parts for final result
	Merge(FixedMem, VarSOut, OutRecMem);
}

///////////////////////////////
/// Field indexer
void TRecIndexer::IndexKey(const TFieldIndexKey& Key, const TMemBase& RecMem,
		const uint64& RecId, TRecSerializator& Serializator) {

	// check the type of field and value to select indexing procedure
	if (Key.FieldType == oftStr && Key.IsValue()){
		// inverted index over non-tokenized strings
		TStr Str = Serializator.GetFieldStr(RecMem, Key.FieldId);
		Index->Index(Key.KeyId, Str, RecId);
	} else if (Key.FieldType == oftStr && Key.IsText()) {
		// inverted index over tokenized strings
		TStr Str = Serializator.GetFieldStr(RecMem, Key.FieldId);
		Index->IndexText(Key.KeyId, Str, RecId);
	} else if (Key.FieldType == oftStrV && Key.IsValue()) {
		// inverted index over string array
		TStrV StrV; Serializator.GetFieldStrV(RecMem, Key.FieldId, StrV);
		Index->Index(Key.KeyId, StrV, RecId);
	} else if (Key.FieldType == oftTm && Key.IsValue()) {
		// time indexed as timestamp string, TODO: proper time indexing
		const uint64 TmMSecs = Serializator.GetFieldTmMSecs(RecMem, Key.FieldId);
		Index->Index(Key.KeyId, TUInt64::GetStr(TmMSecs), RecId);
	} else if (Key.FieldType == oftFltPr && Key.IsLocation()) {
		// index geo-location using geo-index
		TFltPr FltPr = Serializator.GetFieldFltPr(RecMem, Key.FieldId);
		Index->Index(Key.KeyId, FltPr, RecId);
	} else {
		ErrorLog(TStr::Fmt("[TFieldIndexer::IndexKey] Unsupported field and index type combination: %s[%s]: %s",
			Key.FieldNm.CStr(), Key.FieldTypeStr.CStr(), Key.GetKeyType().CStr()));
	}
}

void TRecIndexer::DeindexKey(const TFieldIndexKey& Key, const TMemBase& RecMem,
		const uint64& RecId, TRecSerializator& Serializator) {

	// check the type of field and value to select deindexing procedure
	if (Key.FieldType == oftStr && Key.IsValue()) {
		// inverted index over non-tokenized strings
		TStr Str = Serializator.GetFieldStr(RecMem, Key.FieldId);
		Index->Delete(Key.KeyId, Str, RecId);
	} else if (Key.FieldType == oftStr && Key.IsText()) {
		// inverted index over tokenized strings
		TStr Str = Serializator.GetFieldStr(RecMem, Key.FieldId);
		Index->DeleteText(Key.KeyId, Str, RecId);
	} else if (Key.FieldType == oftStrV && Key.IsValue()) {
		// inverted index over string array
		TStrV StrV; Serializator.GetFieldStrV(RecMem, Key.FieldId, StrV);
		Index->Delete(Key.KeyId, StrV, RecId);
	} else if (Key.FieldType == oftTm && Key.IsValue()) {
		// time indexed as timestamp string, TODO: proper time indexing
		const uint64 TmMSecs = Serializator.GetFieldTmMSecs(RecMem, Key.FieldId);
		Index->Delete(Key.KeyId, TUInt64::GetStr(TmMSecs), RecId);
	} else if (Key.FieldType == oftFltPr && Key.IsLocation()) {
		// index geo-location using geo-index
		TFltPr FltPr = Serializator.GetFieldFltPr(RecMem, Key.FieldId);
		Index->Delete(Key.KeyId, FltPr, RecId);
	} else {
		ErrorLog(TStr::Fmt("[TFieldIndexer::DeindexKey] Unsupported field and index type combination: %s[%s]: %s",
			Key.FieldNm.CStr(), Key.FieldTypeStr.CStr(), Key.GetKeyType().CStr()));
	}
}

void TRecIndexer::UpdateKey(const TFieldIndexKey& Key, const TMemBase& OldRecMem,
	const TMemBase& NewRecMem, const uint64& RecId, TRecSerializator& Serializator) {

	// check the type of field and value to select update procedure
	if (Key.FieldType == oftStr && Key.IsValue()) {
		// inverted index over non-tokenized strings
		TStr OldStr = Serializator.GetFieldStr(OldRecMem, Key.FieldId);
		TStr NewStr = Serializator.GetFieldStr(NewRecMem, Key.FieldId);
		if (OldStr == NewStr) { return; }
		Index->Delete(Key.KeyId, OldStr, RecId);
		Index->Index(Key.KeyId, NewStr, RecId);
	} else if (Key.FieldType == oftStr && Key.IsText()) {
		// inverted index over tokenized strings
		TStr OldStr = Serializator.GetFieldStr(OldRecMem, Key.FieldId);
		TStr NewStr = Serializator.GetFieldStr(NewRecMem, Key.FieldId);
		if (OldStr == NewStr) { return; }
		Index->DeleteText(Key.KeyId, OldStr, RecId);
		Index->IndexText(Key.KeyId, NewStr, RecId);
	} else if (Key.FieldType == oftStrV && Key.IsValue()) {
		// inverted index over string array
		TStrV OldStrV; Serializator.GetFieldStrV(OldRecMem, Key.FieldId, OldStrV);
		TStrV NewStrV; Serializator.GetFieldStrV(NewRecMem, Key.FieldId, NewStrV);
		if (OldStrV == NewStrV) { return; }
		Index->Delete(Key.KeyId, OldStrV, RecId);
		Index->Index(Key.KeyId, NewStrV, RecId);
	} else if (Key.FieldType == oftTm && Key.IsValue()) {
		// time indexed as timestamp string, TODO: proper time indexing
		const uint64 OldTmMSecs = Serializator.GetFieldTmMSecs(OldRecMem, Key.FieldId);
		const uint64 NewTmMSecs = Serializator.GetFieldTmMSecs(NewRecMem, Key.FieldId);
		if (OldTmMSecs == NewTmMSecs) { return; }
		Index->Delete(Key.KeyId, TUInt64::GetStr(OldTmMSecs), RecId);
		Index->Index(Key.KeyId, TUInt64::GetStr(NewTmMSecs), RecId);
	} else if (Key.FieldType == oftFltPr && Key.IsLocation()) {
		// index geo-location using geo-index
		TFltPr OldFltPr = Serializator.GetFieldFltPr(OldRecMem, Key.FieldId);
		TFltPr NewFltPr = Serializator.GetFieldFltPr(NewRecMem, Key.FieldId);
		if (Index->LocEquals(Key.KeyId, OldFltPr, NewFltPr)) { return; }
		Index->Delete(Key.KeyId, OldFltPr, RecId);
		Index->Index(Key.KeyId, NewFltPr, RecId);
	} else {
		ErrorLog(TStr::Fmt("[TFieldIndexer::UpdateKey] Unsupported field and index type combination: %s[%s]: %s",
			Key.FieldNm.CStr(), Key.FieldTypeStr.CStr(), Key.GetKeyType().CStr()));
	}
}

void TRecIndexer::ProcessKey(const TFieldIndexKey& Key, const TMemBase& OldRecMem, 
	const TMemBase& NewRecMem, const uint64& RecId, TRecSerializator& Serializator) {

	// check how to process the change
	const bool OldNullP = Serializator.IsFieldNull(OldRecMem, Key.FieldId);
	const bool NewNullP = Serializator.IsFieldNull(NewRecMem, Key.FieldId);
	if (OldNullP && !NewNullP) {
		// if no value before, just index
		IndexKey(Key, NewRecMem, RecId, Serializator);
	} else if (!OldNullP && NewNullP) {
		// no new value, just deindex
		DeindexKey(Key, OldRecMem, RecId, Serializator);
	} else if (!OldNullP && !NewNullP) {
		// value update, do deindexing of old and indexing of new
		UpdateKey(Key, OldRecMem, NewRecMem, RecId, Serializator);
	} else {
		// nothing before to deindex
		// nothing now to index
		// life is easy
	}
}

TRecIndexer::TRecIndexer(const TWPt<TIndex>& _Index, const TWPt<TStore>& Store):
		Index(_Index), IndexVoc(_Index->GetIndexVoc()) {

	// go over all the fields
	for (int FieldId = 0; FieldId < Store->GetFields(); FieldId++) {
		const TFieldDesc& FieldDesc = Store->GetFieldDesc(FieldId);
		// go over all keys associated with the field
		for (int KeyIdN = 0; KeyIdN < FieldDesc.GetKeys(); KeyIdN++) {
			const int KeyId = FieldDesc.GetKeyId(KeyIdN);
			const TIndexKey& Key = IndexVoc->GetKey(KeyId);
			// remember the field-key details
			const int KeyN = FieldIndexKeyV.Add(TFieldIndexKey(FieldId, 
				FieldDesc.GetFieldNm(), FieldDesc.GetFieldType(), 
				FieldDesc.GetFieldTypeStr(), KeyId, Key.GetTypeFlags(), 
				Key.GetWordVocId()));
			// remember mapping from field id to key position
			FieldIdToKeyN.AddDat(FieldId, KeyN);
		}
	}
}

void TRecIndexer::IndexRec(const TMemBase& RecMem, const uint64& RecId, TRecSerializator& Serializator) {
	// go over all keys associated with the store and its fields
	for (int FieldIndexKeyN = 0; FieldIndexKeyN < FieldIndexKeyV.Len(); FieldIndexKeyN++) {
		const TFieldIndexKey& Key = FieldIndexKeyV[FieldIndexKeyN];
		// check if field is handled by the serializator
		if (!Serializator.IsFieldId(Key.FieldId)) { continue; }
		// check if field is not NULL (e.g. there is something to index)
		if (Serializator.IsFieldNull(RecMem, Key.FieldId)) { continue; }
		// index the key
		IndexKey(Key, RecMem, RecId, Serializator);
	}
}

void TRecIndexer::DeindexRec(const TMemBase& RecMem, const uint64& RecId, TRecSerializator& Serializator) {
	// go over all keys associated with the store and its fields
	for (int FieldIndexKeyN = 0; FieldIndexKeyN < FieldIndexKeyV.Len(); FieldIndexKeyN++) {
		const TFieldIndexKey& Key = FieldIndexKeyV[FieldIndexKeyN];
		// check if field is handled by the serializator
		if (!Serializator.IsFieldId(Key.FieldId)) { continue; }
		// check if field is not NULL (e.g. there is something to deindex)
		if (Serializator.IsFieldNull(RecMem, Key.FieldId)) { continue; }
		// deindex the key
		DeindexKey(Key, RecMem, RecId, Serializator);
	}
}

void TRecIndexer::UpdateRec(const TMemBase& OldRecMem, const TMemBase& NewRecMem,
		const uint64& RecId, const int& ChangedFieldId, TRecSerializator& Serializator) {
	
	// check if we have a key for the field
	if (FieldIdToKeyN.IsKey(ChangedFieldId)) {
		// get field index key
		const int FieldIndexKeyN = FieldIdToKeyN.GetDat(ChangedFieldId);
		const TFieldIndexKey& Key = FieldIndexKeyV[FieldIndexKeyN];
		// check how to process the change
		ProcessKey(Key, OldRecMem, NewRecMem, RecId, Serializator);
	}
}

void TRecIndexer::UpdateRec(const TMemBase& OldRecMem, const TMemBase& NewRecMem,
		const uint64& RecId, TIntSet& ChangedFieldIdSet, TRecSerializator& Serializator) {

	// go over all keys associated with the store and its fields
	for (int FieldIndexKeyN = 0; FieldIndexKeyN < FieldIndexKeyV.Len(); FieldIndexKeyN++) {
		const TFieldIndexKey& Key = FieldIndexKeyV[FieldIndexKeyN];
		// check if field is changed
		if (!ChangedFieldIdSet.IsKey(Key.FieldId)) { continue; }
		// check if field is handled by the serializator
		if (!Serializator.IsFieldId(Key.FieldId)) { continue; }
		// check how to process the change
		ProcessKey(Key, OldRecMem, NewRecMem, RecId, Serializator);
	}
}

bool TRecIndexer::IsFieldIndexKey(const int& FieldId) const {
	// go over all keys associated with the store and its fields
	for (int i = 0; i < FieldIndexKeyV.Len(); i++) {
		if (FieldIndexKeyV[i].FieldId == FieldId) {
			return true;
		}
	}
	return false;
}

///////////////////////////////
/// Implementation of store which can be initialized from a schema.
void TStoreImpl::InitFieldLocV() {
	for (int FieldId = 0; FieldId < GetFields(); FieldId++) {
		if (SerializatorCache->IsFieldId(FieldId)) {
			FieldLocV.Add(slDisk);
		} else if (SerializatorMem->IsFieldId(FieldId)) {
			FieldLocV.Add(slMemory);
		} else {
			throw TQmExcept::New("Unknown storage location for field " +
				GetFieldNm(FieldId) + " in store " + GetStoreNm());
		}
	}
}

void TStoreImpl::GetRecMem(const TStoreLoc& RecLoc, const uint64& RecId, TMem& Rec) const {
	if (RecLoc == slDisk) {
		DataCache.GetVal(RecId, Rec);
	} else if (RecLoc == slMemory)  {
		DataMem.GetVal(RecId, Rec);
	} else {
		throw TQmExcept::New("Unknown storage location");
	}
}

void TStoreImpl::GetRecMem(const uint64& RecId, const int& FieldId, TMem& Rec) const {
	GetRecMem(FieldLocV[FieldId], RecId, Rec);
}

void TStoreImpl::PutRecMem(const TStoreLoc& RecLoc, const uint64& RecId, const TMem& Rec) {
	if (RecLoc == slDisk) {
		DataCache.SetVal(RecId, Rec);
	} else if (RecLoc == slMemory)  {
		DataMem.SetVal(RecId, Rec);
	} else {
		throw TQmExcept::New("Unknown storage location");
	}
}

void TStoreImpl::PutRecMem(const uint64& RecId, const int& FieldId, const TMem& Rec) {
	PutRecMem(FieldLocV[FieldId], RecId, Rec);
}

bool TStoreImpl::IsFieldDisk(const int &FieldId) const {
	return SerializatorCache->IsFieldId(FieldId);
}

bool TStoreImpl::IsFieldInMemory(const int &FieldId) const {
	return SerializatorMem->IsFieldId(FieldId);
}

TRecSerializator* TStoreImpl::GetSerializator(const TStoreLoc& StoreLoc) {
	return (StoreLoc == slMemory) ? SerializatorMem : SerializatorCache;
}

const TRecSerializator* TStoreImpl::GetSerializator(const TStoreLoc& StoreLoc) const {
	return (StoreLoc == slMemory) ? SerializatorMem : SerializatorCache;
}

TRecSerializator* TStoreImpl::GetFieldSerializator(const int &FieldId) {
	return GetSerializator(FieldLocV[FieldId]);
}

const TRecSerializator* TStoreImpl::GetFieldSerializator(const int &FieldId) const {
	return GetSerializator(FieldLocV[FieldId]);
}

void TStoreImpl::SetPrimaryField(const uint64& RecId) {
	if (PrimaryFieldType == oftStr) {
		PrimaryStrIdH.AddDat(GetFieldStr(RecId, PrimaryFieldId)) = RecId;
	} else if (PrimaryFieldType == oftInt) {
		PrimaryIntIdH.AddDat(GetFieldInt(RecId, PrimaryFieldId)) = RecId;
	} else if (PrimaryFieldType == oftUInt64) {
		PrimaryUInt64IdH.AddDat(GetFieldUInt64(RecId, PrimaryFieldId)) = RecId;
	} else if (PrimaryFieldType == oftFlt) {
		PrimaryFltIdH.AddDat(GetFieldFlt(RecId, PrimaryFieldId)) = RecId;
	} else if (PrimaryFieldType == oftTm) {
		PrimaryTmMSecsIdH.AddDat(GetFieldTmMSecs(RecId, PrimaryFieldId)) = RecId;
	}
}

void TStoreImpl::SetPrimaryFieldStr(const uint64& RecId, const TStr& Str) {
    PrimaryStrIdH.AddDat(Str) = RecId;
}

void TStoreImpl::SetPrimaryFieldInt(const uint64& RecId, const int& Int) {
    PrimaryIntIdH.AddDat(Int) = RecId;
}

void TStoreImpl::SetPrimaryFieldUInt64(const uint64& RecId, const uint64& UInt64) {
    PrimaryUInt64IdH.AddDat(UInt64) = RecId;
}

void TStoreImpl::SetPrimaryFieldFlt(const uint64& RecId, const double& Flt) {
    PrimaryFltIdH.AddDat(Flt) = RecId;
}

void TStoreImpl::SetPrimaryFieldMSecs(const uint64& RecId, const uint64& MSecs) {
    PrimaryTmMSecsIdH.AddDat(MSecs) = RecId;
}

void TStoreImpl::DelPrimaryField(const uint64& RecId) {
	if (PrimaryFieldType == oftStr) {
		PrimaryStrIdH.DelIfKey(GetFieldStr(RecId, PrimaryFieldId));
	} else if (PrimaryFieldType == oftInt) {
		PrimaryIntIdH.DelIfKey(GetFieldInt(RecId, PrimaryFieldId));
	} else if (PrimaryFieldType == oftUInt64) {
		PrimaryUInt64IdH.DelIfKey(GetFieldUInt64(RecId, PrimaryFieldId));
	} else if (PrimaryFieldType == oftFlt) {
		PrimaryFltIdH.DelIfKey(GetFieldFlt(RecId, PrimaryFieldId));
	} else if (PrimaryFieldType == oftTm) {
		PrimaryTmMSecsIdH.DelIfKey(GetFieldTmMSecs(RecId, PrimaryFieldId));
	}    
}

void TStoreImpl::DelPrimaryFieldStr(const uint64& RecId, const TStr& Str) {
    Assert(PrimaryStrIdH.GetDat(Str) == RecId);
    PrimaryStrIdH.DelIfKey(Str);
}

void TStoreImpl::DelPrimaryFieldInt(const uint64& RecId, const int& Int) {
    Assert(PrimaryIntIdH.GetDat(Int) == RecId);
    PrimaryIntIdH.DelIfKey(Int);
}

void TStoreImpl::DelPrimaryFieldUInt64(const uint64& RecId, const uint64& UInt64) {
    Assert(PrimaryUInt64IdH.GetDat(UInt64) == RecId);
    PrimaryUInt64IdH.DelIfKey(UInt64);
}

void TStoreImpl::DelPrimaryFieldFlt(const uint64& RecId, const double& Flt) {
    Assert(PrimaryFltIdH.GetDat(Flt) == RecId);
    PrimaryFltIdH.DelIfKey(Flt);
}

void TStoreImpl::DelPrimaryFieldMSecs(const uint64& RecId, const uint64& MSecs) {
    Assert(PrimaryTmMSecsIdH.GetDat(MSecs) == RecId);
    PrimaryTmMSecsIdH.DelIfKey(MSecs);
}

void TStoreImpl::InitFromSchema(const TStoreSchema& StoreSchema) {
	// at start there is no primary key
	RecNmFieldP = false;
	PrimaryFieldId = -1;
	PrimaryFieldType = oftUndef;
	// create fields
	for (int i = 0; i<StoreSchema.FieldH.Len(); i++) {
		const TFieldDesc& FieldDesc = StoreSchema.FieldH[i];
		AddFieldDesc(FieldDesc);
		// check if we found a primary field
		if (FieldDesc.IsPrimary()) {
			QmAssertR(PrimaryFieldId == -1, "Store can have only one primary field");
			// only string fields can serve as record name (TODO: extend)
			RecNmFieldP = FieldDesc.IsStr();
			PrimaryFieldId = GetFieldId(FieldDesc.GetFieldNm());
			PrimaryFieldType = FieldDesc.GetFieldType();
		}
	}
	// create index keys
	TWPt<TIndexVoc> IndexVoc = GetIndex()->GetIndexVoc();
	for (int IndexKeyExN = 0; IndexKeyExN < StoreSchema.IndexKeyExV.Len(); IndexKeyExN++) {
		TIndexKeyEx IndexKeyEx = StoreSchema.IndexKeyExV[IndexKeyExN];
		// get associated field
		const int FieldId = GetFieldId(IndexKeyEx.FieldName);
		// if we are given vocabulary name, check if we have one with such name already
		const int WordVocId = GetBase()->NewIndexWordVoc(IndexKeyEx.KeyType, IndexKeyEx.WordVocName);
		// create new index key
		const int KeyId = GetBase()->NewFieldIndexKey(this, IndexKeyEx.KeyIndexName, 
			FieldId, WordVocId, IndexKeyEx.KeyType, IndexKeyEx.SortType);
		// assign tokenizer to it if we have one
		if (IndexKeyEx.IsTokenizer()) { IndexVoc->PutTokenizer(KeyId, IndexKeyEx.Tokenizer); }
	}
	// prepare serializators for disk and in-memory store
	SerializatorCache = new TRecSerializator(this, StoreSchema, slDisk);
	SerializatorMem = new TRecSerializator(this, StoreSchema, slMemory);
	// initialize field to storage location map
	InitFieldLocV();
	// initialize record indexer
	RecIndexer = TRecIndexer(GetIndex(), this);
	// remember window parameters
	WndDesc =  StoreSchema.WndDesc;
}

void TStoreImpl::InitDataFlags() {
	// go over all the fields and remember if we use in-memory or cache storage
	DataCacheP = false;
	DataMemP = false;
	for (int FieldId = 0; FieldId < GetFields(); FieldId++) {
		DataCacheP = DataCacheP || (FieldLocV[FieldId] == slDisk);
		DataMemP = DataMemP || (FieldLocV[FieldId] == slMemory);
	}    
	// at least one must be true, otherwise we have no fields, which is not good
	EAssert(DataCacheP || DataMemP);    
}

TStoreImpl::TStoreImpl(const TWPt<TBase>& Base, const uint& StoreId, 
	const TStr& StoreName, const TStoreSchema& StoreSchema, const TStr& _StoreFNm, 
	const int64& _MxCacheSize, const int& BlockSize) :
		TStore(Base, StoreId, StoreName), StoreFNm(_StoreFNm), FAccess(faCreate), 
		DataCache(_StoreFNm + ".Cache", _MxCacheSize, 1024), 
		DataMem(_StoreFNm + "MemCache", BlockSize) {

	SetStoreType("TStoreImpl");
	InitFromSchema(StoreSchema);
	// initialize data storage flags
	InitDataFlags();    
}

TStoreImpl::TStoreImpl(const TWPt<TBase>& Base, const TStr& _StoreFNm, 
	const TFAccess& _FAccess, const int64& _MxCacheSize, const bool& _Lazy) :
		TStore(Base, _StoreFNm + ".BaseStore"), 
		StoreFNm(_StoreFNm), FAccess(_FAccess), PrimaryFieldType(oftUndef),
		DataCache(_StoreFNm + ".Cache", _FAccess, _MxCacheSize), 
		DataMem(_StoreFNm + "MemCache", _FAccess, _Lazy) {

	SetStoreType("TStoreImpl");
	// load members
	TFIn FIn(StoreFNm + ".GenericStore");
	RecNmFieldP.Load(FIn);
	PrimaryFieldId.Load(FIn);
	// deduce primary field type
	if (PrimaryFieldId != -1) {
		PrimaryFieldType = GetFieldDesc(PrimaryFieldId).GetFieldType();
		if (PrimaryFieldType == oftStr) {
			PrimaryStrIdH.Load(FIn);
		} else if (PrimaryFieldType == oftInt) {
			PrimaryIntIdH.Load(FIn);
		} else if (PrimaryFieldType == oftUInt64) {
			PrimaryUInt64IdH.Load(FIn);
		} else if (PrimaryFieldType == oftFlt) {
			PrimaryFltIdH.Load(FIn);
		} else if (PrimaryFieldType == oftTm) {
			PrimaryTmMSecsIdH.Load(FIn);
		} else {
			throw TQmExcept::New("Unsupported primary field type!");
		}
	} else {
		// backwards compatibility
		PrimaryStrIdH.Load(FIn);
	}
	// load time window    
	WndDesc.Load(FIn);
	// load data
	SerializatorCache = new TRecSerializator(this);
	SerializatorMem = new TRecSerializator(this);
	SerializatorCache->Load(FIn);
	SerializatorMem->Load(FIn);
	
	// initialize field to storage location map
	InitFieldLocV();
	// initialize record indexer
	RecIndexer = TRecIndexer(GetIndex(), this);
	
	// initialize data storage flags
	InitDataFlags();    
}

TStoreImpl::~TStoreImpl() {
	// save if necessary
	if (FAccess != faRdOnly) {
		TEnv::Logger->OnStatus(TStr::Fmt("Saving store '%s'...", GetStoreNm().CStr()));
		// save base store
		TFOut BaseFOut(StoreFNm + ".BaseStore");
		SaveStore(BaseFOut);
		// save store parameters
		TFOut FOut(StoreFNm + ".GenericStore");
		// save parameters about primary field
		RecNmFieldP.Save(FOut);
		PrimaryFieldId.Save(FOut);
		if (PrimaryFieldType == oftInt) {
			PrimaryIntIdH.Save(FOut);
		} else if (PrimaryFieldType == oftUInt64) {
			PrimaryUInt64IdH.Save(FOut);
		} else if (PrimaryFieldType == oftFlt) {
			PrimaryFltIdH.Save(FOut);
		} else if (PrimaryFieldType == oftTm) {
			PrimaryTmMSecsIdH.Save(FOut);
		} else {
			PrimaryStrIdH.Save(FOut);
		}
		// save time window
		WndDesc.Save(FOut);
		// save data
		SerializatorCache->Save(FOut);
		SerializatorMem->Save(FOut);
	} else {
		TEnv::Logger->OnStatus("No saving of generic store " + GetStoreNm() + " neccessary!");
	}
	delete SerializatorCache;
	delete SerializatorMem;
}

bool TStoreImpl::IsRecId(const uint64& RecId) const { 
	return DataMemP ? DataMem.IsValId(RecId) : DataCache.IsValId(RecId); 
}

uint64 TStoreImpl::GetRecs() const { 
	return DataMemP ? DataMem.Len() : DataCache.Len(); 
}

bool TStoreImpl::IsRecNm(const TStr& RecNm) const { 
	return RecNmFieldP && PrimaryStrIdH.IsKey(RecNm); 
}

TStr TStoreImpl::GetRecNm(const uint64& RecId) const {
	// return empty string when no primary key
	if (!HasRecNm()) { return TStr(); }
	// get the name of primary key
	return GetFieldStr(RecId, PrimaryFieldId);
}

uint64 TStoreImpl::GetRecId(const TStr& RecNm) const {
	return (PrimaryStrIdH.IsKey(RecNm) ? PrimaryStrIdH.GetDat(RecNm) : TUInt64::Mx);
}

PStoreIter TStoreImpl::GetIter() const {
	if (Empty()) { return TStoreIterVec::New(); }
	return DataMemP ? 
		TStoreIterVec::New(DataMem.GetFirstValId(), DataMem.GetLastValId(), true) :
		TStoreIterVec::New(DataCache.GetFirstValId(), DataCache.GetLastValId(), true);
}

uint64 TStoreImpl::FirstRecId() const {
	return Empty() ? TUInt64::Mx.Val : 
		(DataMemP ? DataMem.GetFirstValId() : DataCache.GetFirstValId());
}

uint64 TStoreImpl::LastRecId() const {
	return Empty() ? TUInt64::Mx.Val : 
		(DataMemP ? DataMem.GetLastValId() : DataCache.GetLastValId());
}

PStoreIter TStoreImpl::BackwardIter() const {
	if (Empty()) { return TStoreIterVec::New(); }
	return DataMemP ? 
		TStoreIterVec::New(DataMem.GetLastValId(), DataMem.GetFirstValId(), false) :
		TStoreIterVec::New(DataCache.GetLastValId(), DataCache.GetFirstValId(), false);
}

uint64 TStoreImpl::AddRec(const PJsonVal& RecVal) {
	// check if we are given reference to existing record
	try {        
		// parse out record id, if referred directly
		{
			const uint64 RecId = TStore::GetRecId(RecVal);
			if (IsRecId(RecId)) {
				// check if we have anything more than record identifier, which would require calling UpdateRec
				if (RecVal->GetObjKeys() > 1) { UpdateRec(RecId, RecVal); }
				// return named record
				return RecId;
			}
		}
		// check if we have a primary field
		if (IsPrimaryField()) {
			uint64 PrimaryRecId = TUInt64::Mx;        
			// primary field cannot be nullable, so we must have it
			const TStr& PrimaryField = GetFieldNm(PrimaryFieldId);
			QmAssertR(RecVal->IsObjKey(PrimaryField), "Missing primary field in the record: " + PrimaryField);
			// parse based on the field type
			if (PrimaryFieldType == oftStr) {
				TStr FieldVal = RecVal->GetObjStr(PrimaryField);
				if (PrimaryStrIdH.IsKey(FieldVal)) {
					PrimaryRecId = PrimaryStrIdH.GetDat(FieldVal);
				}
			} else if (PrimaryFieldType == oftInt) {
				const int FieldVal = RecVal->GetObjInt(PrimaryField);
				if (PrimaryIntIdH.IsKey(FieldVal)) {
					PrimaryRecId = PrimaryIntIdH.GetDat(FieldVal);
				}
			} else if (PrimaryFieldType == oftUInt64) {
				const uint64 FieldVal = RecVal->GetObjInt(PrimaryField);
				if (PrimaryUInt64IdH.IsKey(FieldVal)) {
					PrimaryRecId = PrimaryUInt64IdH.GetDat(FieldVal);
				}
			} else if (PrimaryFieldType == oftFlt) {
				const double FieldVal = RecVal->GetObjNum(PrimaryField);
				if (PrimaryFltIdH.IsKey(FieldVal)) {
					PrimaryRecId = PrimaryFltIdH.GetDat(FieldVal);
				}
			} else if (PrimaryFieldType == oftTm) {
				TStr TmStr = RecVal->GetObjStr(PrimaryField);
				TTm Tm = TTm::GetTmFromWebLogDateTimeStr(TmStr, '-', ':', '.', 'T');
				const uint64 FieldVal = TTm::GetMSecsFromTm(Tm);
				if (PrimaryTmMSecsIdH.IsKey(FieldVal)) {
					PrimaryRecId = PrimaryTmMSecsIdH.GetDat(FieldVal);
				}
			}
			// check if we found primary field with existing value
			if (PrimaryRecId != TUInt64::Mx) {
				// check if we have anything more than primary field, which would require redirect to UpdateRec
				if (RecVal->GetObjKeys() > 1) { UpdateRec(PrimaryRecId, RecVal); }
				// return id of named record
				return PrimaryRecId;
			}
		}
	} catch (const PExcept& Except) {
		// error parsing, report error and return nothing
		ErrorLog("[TStoreImpl::AddRec] Error parsing out reference to existing record:");
		ErrorLog(Except->GetMsgStr());
		return TUInt64::Mx;
	}

	// always add system field that means "inserted_at"
	RecVal->AddToObj(TStoreWndDesc::SysInsertedAtFieldName, TTm::GetCurUniTm().GetStr());

	// for storing record id
	uint64 RecId = TUInt64::Mx;    
	uint64 CacheRecId = TUInt64::Mx;    
	uint64 MemRecId = TUInt64::Mx;        
	// store to disk storage
	if (DataCacheP) {
		TMem CacheRecMem; SerializatorCache->Serialize(RecVal, CacheRecMem, this);
		CacheRecId = DataCache.AddVal(CacheRecMem);
		RecId = CacheRecId;
		// index new record
		RecIndexer.IndexRec(CacheRecMem, RecId, *SerializatorCache);
	}
	// store to in-memory storage
	if (DataMemP) {
		TMem MemRecMem; SerializatorMem->Serialize(RecVal, MemRecMem, this);
		MemRecId = DataMem.AddVal(MemRecMem);
		RecId = MemRecId;
		// index new record
		RecIndexer.IndexRec(MemRecMem, RecId, *SerializatorMem);
	}
	// make sure we are consistent with respect to Ids!
	if (DataCacheP && DataMemP) {
		EAssert(CacheRecId == MemRecId);
	}
	
	// remember value-recordId map when primary field available
	if (IsPrimaryField()) { SetPrimaryField(RecId); }
	
	// insert nested join records
	AddJoinRec(RecId, RecVal);
	// call add triggers
	OnAdd(RecId);
	
	// return record Id of the new record
	return RecId;
}

void TStoreImpl::UpdateRec(const uint64& RecId, const PJsonVal& RecVal) {    
	// figure out which storage fields are affected
	bool CacheP = false, MemP = false, PrimaryP = false;
	for (int FieldId = 0; FieldId < GetFields(); FieldId++) {
		// check if field appears in the record JSon
		TStr FieldNm = GetFieldNm(FieldId);
		if (RecVal->IsObjKey(FieldNm)) {
			CacheP = CacheP || (FieldLocV[FieldId] == slDisk);
			MemP = MemP || (FieldLocV[FieldId] == slMemory);
			PrimaryP = PrimaryP || (FieldId == PrimaryFieldId);
		}
	}
	// remove old primary field
	if (PrimaryP) { DelPrimaryField(RecId); }
	// update disk serialization when necessary
	if (CacheP) {
		// update serialization
		TMem CacheOldRecMem; DataCache.GetVal(RecId, CacheOldRecMem);
		TMem CacheNewRecMem; TIntSet CacheChangedFieldIdSet;
		SerializatorCache->SerializeUpdate(RecVal, CacheOldRecMem,
			CacheNewRecMem, this, CacheChangedFieldIdSet);
		// update the stored serializations with new values
		DataCache.SetVal(RecId, CacheNewRecMem);
		// update indexes pointing to the record
		RecIndexer.UpdateRec(CacheOldRecMem, CacheNewRecMem, RecId, CacheChangedFieldIdSet, *SerializatorCache);
	}
	// update in-memory serialization when necessary
	if (MemP) {
		// update serialization
		TMem MemOldRecMem; DataMem.GetVal(RecId, MemOldRecMem);
		TMem MemNewRecMem; TIntSet MemChangedFieldIdSet;
		SerializatorMem->SerializeUpdate(RecVal, MemOldRecMem,
			MemNewRecMem, this, MemChangedFieldIdSet);
		// update the stored serializations with new values
		DataMem.SetVal(RecId, MemNewRecMem);
		// update indexes pointing to the record
		RecIndexer.UpdateRec(MemOldRecMem, MemNewRecMem, RecId, MemChangedFieldIdSet, *SerializatorMem);
	}
	// check if primary key changed and update the mapping
	if (PrimaryP) { SetPrimaryField(RecId); }
	// call update triggers
	OnUpdate(RecId);
}

void TStoreImpl::GarbageCollect() {
	// if no window, nothing to do here
	if (WndDesc.WindowType == swtNone) { return; }
	// if no records, nothing to do here
	if (Empty()) { return; }
	// report on activity
	TEnv::Logger->OnStatusFmt("Garbage Collection in %s", GetStoreNm().CStr());
	TEnv::Logger->OnStatusFmt("  %s records at start", TUInt64::GetStr(GetRecs()).CStr());

	// prepare list of records that need to be deleted
	TUInt64V DelRecIdV;
	if (WndDesc.WindowType == swtTime) {
		// get last added record
		const uint64 LastRecId = DataCache.GetLastValId();
		// not sure why this here, report as error for now
		if (!IsRecId(LastRecId)) { ErrorLog("Invalid last record in garbage collection"); return; }
		// get time window field
		const int TimeFieldId = GetFieldId(WndDesc.TimeFieldNm);
		// get time which we use as end of time-window (could be insert time or field value)
		uint64 CurMSecs = WndDesc.InsertP ? TTm::GetCurUniMSecs() :
			GetFieldTmMSecs(LastRecId, TimeFieldId);
		// get start of time window
		const uint64 WindowStartMSecs = CurMSecs - WndDesc.WindowSize;
		// report what is the established time window used by the garbage collection
		TEnv::Logger->OnStatusFmt("  window: %s - %s",
			TTm::GetTmFromMSecs(WindowStartMSecs).GetWebLogDateTimeStr(true, "T", false).CStr(),
			TTm::GetTmFromMSecs(CurMSecs).GetWebLogDateTimeStr(true, "T", false).CStr());
		// iterate from the start until we hit the time window
		PStoreIter Iter = GetIter();        
		while (Iter->Next()) {
			uint64 RecId = Iter->GetRecId();
			// get record time
			uint64 TmMSecs = GetFieldTmMSecs(RecId, TimeFieldId);
			// if we are within time window we stop
			if (TmMSecs >= WindowStartMSecs) break;
			// otherwise we mark the record for deletion
			DelRecIdV.Add(RecId);
		}
		// report progress
	} else if (GetRecs() > WndDesc.WindowSize) {
		// we are windowing based on number of records
		TEnv::Logger->OnStatusFmt("  window: last %d records", (int)WndDesc.WindowSize);
		// get number of records which need to be deleted so we are back in the window
		int DelRecs = (int)(GetRecs() - WndDesc.WindowSize);
		// iterate from the start until we hit the time window
		PStoreIter Iter = GetIter();
		while (Iter->Next() && DelRecs > 0) {
			// mark record for deletion
			DelRecIdV.Add(Iter->GetRecId());
			// track progress
			DelRecs--;
		}
	}
	TEnv::Logger->OnStatusFmt("  purging %d records", DelRecIdV.Len());
	TStoreImpl::DeleteRecs(DelRecIdV, false);    
}

/// Deletes all records
void TStoreImpl::DeleteAllRecs() {
	// if no records, nothing to do here
	if (Empty()) { return; }
	TEnv::Logger->OnStatusFmt("Deleting all (%d) records in %s", GetRecs(), GetStoreNm().CStr());
	
	// delete records from index
	for (uint64 DelRecId = GetFirstRecId(); DelRecId <= GetLastRecId(); DelRecId++) {
		// executed triggers before deletion
		OnDelete(DelRecId);
		// delete record from name-id map
		if (IsPrimaryField()) { DelPrimaryField(DelRecId); }
		// delete record from indexes
		if (DataCacheP) {
			TMem CacheRecMem; 
			DataCache.GetVal(DelRecId, CacheRecMem);
			RecIndexer.DeindexRec(CacheRecMem, DelRecId, *SerializatorCache);
		}
		if (DataMemP) {
			TMem MemRecMem; 
			DataMem.GetVal(DelRecId, MemRecMem);
			RecIndexer.DeindexRec(MemRecMem, DelRecId, *SerializatorMem);
		}
		// delete record from joins
		TRec Rec(this, DelRecId);
		for (int JoinN = 0; JoinN < GetJoins(); JoinN++) {
			TJoinDesc JoinDesc = GetJoinDesc(JoinN);
			// execute the join
			PRecSet JoinRecSet = Rec.DoJoin(GetBase(), JoinDesc.GetJoinId());
			for (int JoinRecN = 0; JoinRecN < JoinRecSet->GetRecs(); JoinRecN++) {
				// remove joins with all matched records, one by one
				const uint64 JoinRecId = JoinRecSet->GetRecId(JoinRecN);
				const int JoinFq = JoinRecSet->GetRecFq(JoinRecN);
				DelJoin(JoinDesc.GetJoinId(), DelRecId, JoinRecId, JoinFq);
			}
		}
	}
	// delete records from disk
	PrimaryStrIdH.Clr();
	PrimaryIntIdH.Clr();
	PrimaryUInt64IdH.Clr();
	PrimaryFltIdH.Clr();
	PrimaryTmMSecsIdH.Clr();
	DataCache.DelVals(TInt::Mx);
	DataMem.DelVals(TInt::Mx);
	PartialFlush(TInt::Mx);
}

void TStoreImpl::DeleteFirstNRecs(int DelRecs)  {
	// if no records, nothing to do here
	if (Empty()) { return; }
	// report on activity
	TEnv::Logger->OnStatusFmt("Deleting %d records in %s", DelRecs, GetStoreNm().CStr());
	TEnv::Logger->OnStatusFmt("  %s records at start", TUInt64::GetStr(GetRecs()).CStr());

	// prepare list of records that need to be deleted
	TUInt64V DelRecIdV;
	// iterate from the start until we hit the time window
	PStoreIter Iter = GetIter();
	while (Iter->Next() && DelRecs > 0) {
		// mark record for deletion
		DelRecIdV.Add(Iter->GetRecId());
		// track progress
		DelRecs--;
	}
	TStoreImpl::DeleteRecs(DelRecIdV, false);
}

void TStoreImpl::DeleteRecs(const TUInt64V& DelRecIdV, const bool& AssertOK) {
	if (AssertOK) {
		// assert that DelRecIdV is valid, without gaps and that deleting will not create gaps
		PStoreIter Iter = GetIter();
		int Counter = 0;
		QmAssertR((uint64)DelRecIdV.Len() <= GetRecs(), "TStoreImpl::DeleteRecs incorrect record id sequence. The length is greater than the total number of records.");
		while (Iter->Next()) {
			QmAssertR(DelRecIdV[Counter] == Iter->GetRecId(), "TStoreImpl::DeleteRecs: incorrect record id sequence. The sequence should start at the first store records, should contain only record ids and should not contain gaps");
			Counter++;
		}
	}
	// delete records from index
	for (int DelRecN = 0; DelRecN < DelRecIdV.Len(); DelRecN++) {
		// report progress
		if (DelRecN % 1000 == 0) { TEnv::Logger->OnStatusFmt("    %d\r", DelRecN); }
		// what are we deleting now
		const uint64 DelRecId = DelRecIdV[DelRecN];
		// executed triggers before deletion
		OnDelete(DelRecId);
		// delete record from name-id map
		if (IsPrimaryField()) { DelPrimaryField(DelRecId); }
		// delete record from indexes
		if (DataCacheP) {
			TMem CacheRecMem; DataCache.GetVal(DelRecId, CacheRecMem);
			RecIndexer.DeindexRec(CacheRecMem, DelRecId, *SerializatorCache);
		}
		if (DataMemP) {
			TMem MemRecMem; DataMem.GetVal(DelRecId, MemRecMem);
			RecIndexer.DeindexRec(MemRecMem, DelRecId, *SerializatorMem);
		}
		// delete record from joins
		TRec Rec(this, DelRecId);
		for (int JoinN = 0; JoinN < GetJoins(); JoinN++) {
			TJoinDesc JoinDesc = GetJoinDesc(JoinN);
			// execute the join
			PRecSet JoinRecSet = Rec.DoJoin(GetBase(), JoinDesc.GetJoinId());
			for (int JoinRecN = 0; JoinRecN < JoinRecSet->GetRecs(); JoinRecN++) {
				// remove joins with all matched records, one by one
				const uint64 JoinRecId = JoinRecSet->GetRecId(JoinRecN);
				const int JoinFq = JoinRecSet->GetRecFq(JoinRecN);
				DelJoin(JoinDesc.GetJoinId(), DelRecId, JoinRecId, JoinFq);
			}
		}
	}
	// delete records from disk
	if (DataCacheP) { DataCache.DelVals(DelRecIdV.Len()); }
	// delete records from in-memory store
	if (DataMemP) { DataMem.DelVals(DelRecIdV.Len()); }

	// report success :-)
	TEnv::Logger->OnStatusFmt("  %s records at end", TUInt64::GetStr(GetRecs()).CStr());
}

bool TStoreImpl::IsFieldNull(const uint64& RecId, const int& FieldId) const {
	TMem RecMem; GetRecMem(RecId, FieldId, RecMem);
	return GetFieldSerializator(FieldId)->IsFieldNull(RecMem, FieldId);
}

int TStoreImpl::GetFieldInt(const uint64& RecId, const int& FieldId) const {
	TMem RecMem; GetRecMem(RecId, FieldId, RecMem);
	return GetFieldSerializator(FieldId)->GetFieldInt(RecMem, FieldId);
}

TStr TStoreImpl::GetFieldStr(const uint64& RecId, const int& FieldId) const {
	TMem RecMem; GetRecMem(RecId, FieldId, RecMem);
	return GetFieldSerializator(FieldId)->GetFieldStr(RecMem, FieldId);
}

bool TStoreImpl::GetFieldBool(const uint64& RecId, const int& FieldId) const {
	TMem RecMem; GetRecMem(RecId, FieldId, RecMem);
	return GetFieldSerializator(FieldId)->GetFieldBool(RecMem, FieldId);
}

double TStoreImpl::GetFieldFlt(const uint64& RecId, const int& FieldId) const {
	TMem RecMem; GetRecMem(RecId, FieldId, RecMem);
	return GetFieldSerializator(FieldId)->GetFieldFlt(RecMem, FieldId);
}

TFltPr TStoreImpl::GetFieldFltPr(const uint64& RecId, const int& FieldId) const {
	TMem RecMem; GetRecMem(RecId, FieldId, RecMem);
	return GetFieldSerializator(FieldId)->GetFieldFltPr(RecMem, FieldId);
}

uint64 TStoreImpl::GetFieldUInt64(const uint64& RecId, const int& FieldId) const {
	TMem RecMem; GetRecMem(RecId, FieldId, RecMem);
	return GetFieldSerializator(FieldId)->GetFieldUInt64(RecMem, FieldId);
}

void TStoreImpl::GetFieldStrV(const uint64& RecId, const int& FieldId, TStrV& StrV) const {
	TMem RecMem; GetRecMem(RecId, FieldId, RecMem);
	GetFieldSerializator(FieldId)->GetFieldStrV(RecMem, FieldId, StrV);
}

void TStoreImpl::GetFieldIntV(const uint64& RecId, const int& FieldId, TIntV& IntV) const {
	TMem RecMem; GetRecMem(RecId, FieldId, RecMem);
	GetFieldSerializator(FieldId)->GetFieldIntV(RecMem, FieldId, IntV);
}

void TStoreImpl::GetFieldFltV(const uint64& RecId, const int& FieldId, TFltV& FltV) const {
	TMem RecMem; GetRecMem(RecId, FieldId, RecMem);
	GetFieldSerializator(FieldId)->GetFieldFltV(RecMem, FieldId, FltV);
}

void TStoreImpl::GetFieldTm(const uint64& RecId, const int& FieldId, TTm& Tm) const {
	TMem RecMem; GetRecMem(RecId, FieldId, RecMem);
	GetFieldSerializator(FieldId)->GetFieldTm(RecMem, FieldId, Tm);
}

uint64 TStoreImpl::GetFieldTmMSecs(const uint64& RecId, const int& FieldId) const {
	TMem RecMem; GetRecMem(RecId, FieldId, RecMem);
	return GetFieldSerializator(FieldId)->GetFieldTmMSecs(RecMem, FieldId);
}

void TStoreImpl::GetFieldNumSpV(const uint64& RecId, const int& FieldId, TIntFltKdV& SpV) const {
	TMem RecMem; GetRecMem(RecId, FieldId, RecMem);
	GetFieldSerializator(FieldId)->GetFieldNumSpV(RecMem, FieldId, SpV);
}

void TStoreImpl::GetFieldBowSpV(const uint64& RecId, const int& FieldId, PBowSpV& SpV) const {
	TMem RecMem; GetRecMem(RecId, FieldId, RecMem);
	GetFieldSerializator(FieldId)->GetFieldBowSpV(RecMem, FieldId, SpV);
}

void TStoreImpl::SetFieldNull(const uint64& RecId, const int& FieldId) {
	TMem InRecMem; GetRecMem(RecId, FieldId, InRecMem);
	TRecSerializator* FieldSerializator = GetFieldSerializator(FieldId);
	TMem OutRecMem; FieldSerializator->SetFieldNull(InRecMem, OutRecMem, FieldId);
	RecIndexer.UpdateRec(InRecMem, OutRecMem, RecId, FieldId, *FieldSerializator);
	PutRecMem(RecId, FieldId, OutRecMem);
}

void TStoreImpl::SetFieldInt(const uint64& RecId, const int& FieldId, const int& Int) {
	TMem InRecMem; GetRecMem(RecId, FieldId, InRecMem);
	TRecSerializator* FieldSerializator = GetFieldSerializator(FieldId);
    if (FieldId == PrimaryFieldId) {
		DelPrimaryFieldInt(RecId, FieldSerializator->GetFieldInt(InRecMem, FieldId));
	}
	TMem OutRecMem; 
	FieldSerializator->SetFieldInt(InRecMem, OutRecMem, FieldId, Int);
	RecIndexer.UpdateRec(InRecMem, OutRecMem, RecId, FieldId, *FieldSerializator);
	PutRecMem(RecId, FieldId, OutRecMem);
    if (FieldId == PrimaryFieldId) { SetPrimaryFieldInt(RecId, Int); }
}

void TStoreImpl::SetFieldIntV(const uint64& RecId, const int& FieldId, const TIntV& IntV) {
	TMem InRecMem; GetRecMem(RecId, FieldId, InRecMem);
	TRecSerializator* FieldSerializator = GetFieldSerializator(FieldId);
	TMem OutRecMem;
	FieldSerializator->SetFieldIntV(InRecMem, OutRecMem, FieldId, IntV);
	RecIndexer.UpdateRec(InRecMem, OutRecMem, RecId, FieldId, *FieldSerializator);
	PutRecMem(RecId, FieldId, OutRecMem);
}

void TStoreImpl::SetFieldUInt64(const uint64& RecId, const int& FieldId, const uint64& UInt64) {
	TMem InRecMem; GetRecMem(RecId, FieldId, InRecMem);
	TRecSerializator* FieldSerializator = GetFieldSerializator(FieldId);
    if (FieldId == PrimaryFieldId) {
		DelPrimaryFieldUInt64(RecId, FieldSerializator->GetFieldUInt64(InRecMem, FieldId));
	}
	TMem OutRecMem;
	FieldSerializator->SetFieldUInt64(InRecMem, OutRecMem, FieldId, UInt64);
	RecIndexer.UpdateRec(InRecMem, OutRecMem, RecId, FieldId, *FieldSerializator);
	PutRecMem(RecId, FieldId, OutRecMem);
    if (FieldId == PrimaryFieldId) { SetPrimaryFieldUInt64(RecId, UInt64); }
}

void TStoreImpl::SetFieldStr(const uint64& RecId, const int& FieldId, const TStr& Str) {
	TMem InRecMem; GetRecMem(RecId, FieldId, InRecMem);
	TRecSerializator* FieldSerializator = GetFieldSerializator(FieldId);
    if (FieldId == PrimaryFieldId) {
		DelPrimaryFieldStr(RecId, FieldSerializator->GetFieldStr(InRecMem, FieldId));
	}
	TMem OutRecMem;
	FieldSerializator->SetFieldStr(InRecMem, OutRecMem, FieldId, Str);
	RecIndexer.UpdateRec(InRecMem, OutRecMem, RecId, FieldId, *FieldSerializator);
	PutRecMem(RecId, FieldId, OutRecMem);
    if (FieldId == PrimaryFieldId) { SetPrimaryFieldStr(RecId, Str); }
}

void TStoreImpl::SetFieldStrV(const uint64& RecId, const int& FieldId, const TStrV& StrV) {
	TMem InRecMem; GetRecMem(RecId, FieldId, InRecMem);
	TRecSerializator* FieldSerializator = GetFieldSerializator(FieldId);
	TMem OutRecMem; 
	FieldSerializator->SetFieldStrV(InRecMem, OutRecMem, FieldId, StrV);
	RecIndexer.UpdateRec(InRecMem, OutRecMem, RecId, FieldId, *FieldSerializator);
	PutRecMem(RecId, FieldId, OutRecMem);
}

void TStoreImpl::SetFieldBool(const uint64& RecId, const int& FieldId, const bool& Bool) {
	TMem InRecMem; GetRecMem(RecId, FieldId, InRecMem);
	TRecSerializator* FieldSerializator = GetFieldSerializator(FieldId);
	TMem OutRecMem; 
	FieldSerializator->SetFieldBool(InRecMem, OutRecMem, FieldId, Bool);
	RecIndexer.UpdateRec(InRecMem, OutRecMem, RecId, FieldId, *FieldSerializator);
	PutRecMem(RecId, FieldId, OutRecMem);
}

void TStoreImpl::SetFieldFlt(const uint64& RecId, const int& FieldId, const double& Flt) {
	TMem InRecMem; GetRecMem(RecId, FieldId, InRecMem);
	TRecSerializator* FieldSerializator = GetFieldSerializator(FieldId);
    if (FieldId == PrimaryFieldId) {
		DelPrimaryFieldFlt(RecId, FieldSerializator->GetFieldFlt(InRecMem, FieldId));
	}
	TMem OutRecMem;
	FieldSerializator->SetFieldFlt(InRecMem, OutRecMem, FieldId, Flt);
	RecIndexer.UpdateRec(InRecMem, OutRecMem, RecId, FieldId, *FieldSerializator);
	PutRecMem(RecId, FieldId, OutRecMem);
    if (FieldId == PrimaryFieldId) { SetPrimaryFieldFlt(RecId, Flt); }
}

void TStoreImpl::SetFieldFltPr(const uint64& RecId, const int& FieldId, const TFltPr& FltPr) {
	TMem InRecMem; GetRecMem(RecId, FieldId, InRecMem);
	TRecSerializator* FieldSerializator = GetFieldSerializator(FieldId);
	TMem OutRecMem;
	FieldSerializator->SetFieldFltPr(InRecMem, OutRecMem, FieldId, FltPr);
	RecIndexer.UpdateRec(InRecMem, OutRecMem, RecId, FieldId, *FieldSerializator);
	PutRecMem(RecId, FieldId, OutRecMem);
}

void TStoreImpl::SetFieldFltV(const uint64& RecId, const int& FieldId, const TFltV& FltV) {
	TMem InRecMem; GetRecMem(RecId, FieldId, InRecMem);
	TRecSerializator* FieldSerializator = GetFieldSerializator(FieldId);
	TMem OutRecMem;
	FieldSerializator->SetFieldFltV(InRecMem, OutRecMem, FieldId, FltV);
	RecIndexer.UpdateRec(InRecMem, OutRecMem, RecId, FieldId, *FieldSerializator);
	PutRecMem(RecId, FieldId, OutRecMem);
}

void TStoreImpl::SetFieldTm(const uint64& RecId, const int& FieldId, const TTm& Tm) {
	TMem InRecMem; GetRecMem(RecId, FieldId, InRecMem);
	TRecSerializator* FieldSerializator = GetFieldSerializator(FieldId);
	TMem OutRecMem;
	FieldSerializator->SetFieldTm(InRecMem, OutRecMem, FieldId, Tm);
	RecIndexer.UpdateRec(InRecMem, OutRecMem, RecId, FieldId, *FieldSerializator);
	PutRecMem(RecId, FieldId, OutRecMem);
}

void TStoreImpl::SetFieldTmMSecs(const uint64& RecId, const int& FieldId, const uint64& TmMSecs) {
	TMem InRecMem; GetRecMem(RecId, FieldId, InRecMem);
	TRecSerializator* FieldSerializator = GetFieldSerializator(FieldId);
    if (FieldId == PrimaryFieldId) {
		DelPrimaryFieldMSecs(RecId, FieldSerializator->GetFieldTmMSecs(InRecMem, FieldId));
	}
	TMem OutRecMem;
	FieldSerializator->SetFieldTmMSecs(InRecMem, OutRecMem, FieldId, TmMSecs);
	RecIndexer.UpdateRec(InRecMem, OutRecMem, RecId, FieldId, *FieldSerializator);
	PutRecMem(RecId, FieldId, OutRecMem);
    if (FieldId == PrimaryFieldId) { SetPrimaryFieldMSecs(RecId, TmMSecs); }
}

void TStoreImpl::SetFieldNumSpV(const uint64& RecId, const int& FieldId, const TIntFltKdV& SpV) {
	TMem InRecMem; GetRecMem(RecId, FieldId, InRecMem);
	TRecSerializator* FieldSerializator = GetFieldSerializator(FieldId);
	TMem OutRecMem; 
	FieldSerializator->SetFieldNumSpV(InRecMem, OutRecMem, FieldId, SpV);
	RecIndexer.UpdateRec(InRecMem, OutRecMem, RecId, FieldId, *FieldSerializator);
	PutRecMem(RecId, FieldId, OutRecMem);
}

void TStoreImpl::SetFieldBowSpV(const uint64& RecId, const int& FieldId, const PBowSpV& SpV) {
	TMem InRecMem; GetRecMem(RecId, FieldId, InRecMem);
	TRecSerializator* FieldSerializator = GetFieldSerializator(FieldId);
	TMem OutRecMem; 
	FieldSerializator->SetFieldBowSpV(InRecMem, OutRecMem, FieldId, SpV);
	RecIndexer.UpdateRec(InRecMem, OutRecMem, RecId, FieldId, *FieldSerializator);
	PutRecMem(RecId, FieldId, OutRecMem);
}

PJsonVal TStoreImpl::GetStoreJson(const TWPt<TBase>& Base) const {
	PJsonVal Result = TStore::GetStoreJson(Base);

	if (WndDesc.WindowType != TStoreWndType::swtNone) {
		PJsonVal WindowJson = TJsonVal::NewObj();

		WindowJson->AddToObj("type", WndDesc.WindowType == TStoreWndType::swtLength ? "length" : "time");
		WindowJson->AddToObj("size", (int) WndDesc.WindowSize);

		if (WndDesc.WindowType == TStoreWndType::swtTime) {
			WindowJson->AddToObj("timeField", WndDesc.TimeFieldNm);
		}

		Result->AddToObj("window", WindowJson);
	}

	return Result;
}


/// Save part of the data, given time-window
int TStoreImpl::PartialFlush(int WndInMsec) {
	int slice = WndInMsec / 2;
	TTmStopWatch sw(true);
	int res = DataMem.PartialFlush(slice);
	res += DataCache.PartialFlush(slice);
	return res;
}

/// Retrieve performance statistics for this store
PJsonVal TStoreImpl::GetStats() {
	PJsonVal res = TJsonVal::NewObj();
	res->AddToObj("name", GetStoreNm());
	res->AddToObj("blob_storage_memory", BlobBsStatsToJson(DataMem.GetBlobBsStats()));
	res->AddToObj("blob_storage_cache", BlobBsStatsToJson(DataCache.GetBlobBsStats()));
	return res;
}

} // TStorage

}
