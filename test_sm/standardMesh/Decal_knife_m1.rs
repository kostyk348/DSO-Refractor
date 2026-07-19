subshader "Decal_knife_m1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 0.388235 0.388235 0.388235;
	blendSrc sourceAlpha; 
	blendDest invsourceAlpha;
	transparent true;
	depthWrite false;
	alphaTestRef 0.5;
	texture "texture/decal_knife_I";
}

