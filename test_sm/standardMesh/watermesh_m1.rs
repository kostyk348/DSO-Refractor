subshader "watermesh_m1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	transparent true;
	twosided true;
	blendSrc sourceAlpha; 
	blendDest one;
	depthWrite false;
	alphaTestRef 0.7;
	texture "texture/water_ono_I";
}

