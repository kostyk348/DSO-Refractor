subshader "shockwave_m1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	blendSrc sourceAlpha; 
	blendDest one;
	transparent true;
	depthWrite false;
	alphaTestRef 0.5;
	texture "texture/shockwave_I";
}

