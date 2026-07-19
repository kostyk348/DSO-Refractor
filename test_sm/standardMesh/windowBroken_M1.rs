subshader "windowBroken_M1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 0.196078 0.196078 0.196078;
	materialSpecular 0.0588235 0.0588235 0.0588235;
	materialSpecularPower 12.5;
	texture "texture/windowframe_I";
}

subshader "windowBroken_M1_Material1" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 0.427451 0.427451 0.427451;
	materialSpecular 0.329412 0.329412 0.329412;
	materialSpecularPower 12.5;
	transparent true;
	twosided true;
	envmap true;
	texture "texture/windowglass_broken_I";
}

