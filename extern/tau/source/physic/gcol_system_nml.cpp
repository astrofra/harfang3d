/*

GCollide	Collision library serialisation.
			Written by Emmanuel Julien.
			All rights reserved 2000~2005.

			Emmanuel Julien
			http://www.nengine.fr
			mailto:ejulien@nengine.fr

*/


		#include	"physic.h"


//-----------------------------------------------------------------------------------
bool		GCollide::FromMetaTag(nMetaFile &file, nMetaTag &tag, bool load_settings)
//-----------------------------------------------------------------------------------
{
	if	(tag.id != "GCollide")
	{
		__LOG_E__ << "could not parse collision system. Root tag id is incorrect (" << tag.id.c_str() << ").\n";
		return false;
	}
#ifdef __GCOL_ENABLE_SAP__
	if	(load_settings)
	{
		nMetaTag	*tag = file.GetTag("UseSAP");
		use_sap = tag ? tag->GetBool() : true;
	}
	if	(use_sap && load_settings)
	{
		nMetaTag	*tag = file.GetTypedTag("SAPSize", nMetaTag::Type_Integer);
		if	(tag)
			SAP_Allocate(tag->GetInteger());
	}
#endif
	return true;
}


//-----------------------------------------------------
nMetaTag	*GCollide::AsMetaTag(nMetaFile &file) const
//-----------------------------------------------------
{
	nMetaTag	*root = new nMetaTag("GCollide");
	if	(!root)
	{
		__LOG_E__ << "could not create gcollide root tag to serialize.\n";
		return NULL;
	}

#ifdef __GCOL_ENABLE_SAP__
	if	(use_sap)
	{
		root->AddChild(new nMetaTag("UseSAP", use_sap));
		root->AddChild(new nMetaTag("SAPSize", (int)SAP_totalsize));
	}
#endif
	return root;
}
