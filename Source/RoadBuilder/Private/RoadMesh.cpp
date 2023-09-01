// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "RoadMesh.h"
#include "Settings.h"
#include "Components/DecalComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#ifndef M_PI
	#define M_PI    3.14159265358979323846
#endif
#pragma warning(disable:4456)
#include "../../ThirdParty/CDT/include/CDT.h"

FStaticRoadMesh::FStaticRoadMesh()
{
	MeshDescription = MakeShareable(new FMeshDescription);
	MeshAttributes = MakeShareable(new FStaticMeshAttributes(*MeshDescription.Get()));
	MeshAttributes->Register();
	Builder = MakeShareable(new FMeshDescriptionBuilder);
	Builder->SetMeshDescription(MeshDescription.Get());
	Builder->EnablePolyGroups();
	Builder->SetNumUVLayers(1);
}

UStaticMesh* FStaticRoadMesh::CreateMesh(UObject* Outer, FName Name, EObjectFlags Flags)
{
	UStaticMesh* Mesh = nullptr;
	if (MeshDescription->Triangles().Num())
	{
		Mesh = NewObject<UStaticMesh>(Outer, Name, Flags);
		TArray<FStaticMaterial>& StaticMaterials = Mesh->GetStaticMaterials();
		for (auto KV : PolygonGroups)
		{
			FStaticMaterial& StaticMaterial = StaticMaterials[StaticMaterials.AddDefaulted()];
			StaticMaterial.MaterialInterface = KV.Key;
			StaticMaterial.MaterialSlotName = KV.Key ? KV.Key->GetFName() : NAME_None;
			StaticMaterial.UVChannelData = FMeshUVChannelInfo(1.f);
		}
		TArray<const FMeshDescription*> Descs = { MeshDescription.Get() };
		UStaticMesh::FBuildMeshDescriptionsParams Params;
		Params.bFastBuild = true;
		Params.bAllowCpuAccess = true;
	//	Params.bCommitMeshDescription = false;
		Params.PerLODOverrides = { {true, true} };
		Mesh->BuildFromMeshDescriptions(Descs, Params);
		if (!Mesh->GetBodySetup())
			Mesh->CreateBodySetup();
		Mesh->GetBodySetup()->CollisionTraceFlag = CTF_UseComplexAsSimple;
	//	Mesh->GetBodySetup()->DefaultInstance.SetCollisionProfileName(TEXT("NoCollision"));
	}
	return Mesh;
}

FPolygonGroupID FStaticRoadMesh::GetGroupID(UMaterialInterface* Material)
{
	if (!PolygonGroups.Contains(Material))
	{
		FPolygonGroupID Group = Builder->AppendPolygonGroup(Material ? Material->GetFName() : NAME_None);
		PolygonGroups.Add(Material, Group);
	}
	return PolygonGroups[Material];
}

void FStaticRoadMesh::AddStrip(UMaterialInterface* Material, const FPolyline& LeftCurve, const FPolyline& RightCurve)
{
	SCOPE_CYCLE_COUNTER(STAT_AddStrip);
	FPolygonGroupID Group = GetGroupID(Material);
	TArray<FVertexID> VertexIDs;
	VertexIDs.AddUninitialized(LeftCurve.Points.Num() + RightCurve.Points.Num());
	for (int i = 0; i < LeftCurve.Points.Num(); i++)
		VertexIDs[i] = Builder->AppendVertex(LeftCurve.Points[i].Pos);
	for (int i = 0; i < RightCurve.Points.Num(); i++)
		VertexIDs[i + LeftCurve.Points.Num()] = Builder->AppendVertex(RightCurve.Points[i].Pos);
	TArray<FVertexInstanceID> InstanceIDs;
	InstanceIDs.AddUninitialized((LeftCurve.Points.Num() + RightCurve.Points.Num() - 2) * 3);
	int NumTriangles = 0;
	double UVScale = GetMutableDefault<USettings_Global>()->UVScale;
	auto AddTriangle = [&](int LeftStart, int RightStart, bool LeftSide)
	{
		const FVector& P0 = LeftCurve.Points[LeftStart].Pos;
		const FVector& P1 = RightCurve.Points[RightStart].Pos;
		const FVector& P2 = LeftSide ? LeftCurve.Points[LeftStart + 1].Pos : RightCurve.Points[RightStart + 1].Pos;
		FVector Cross = (P2 - P0).GetSafeNormal() ^ (P1 - P0).GetSafeNormal();
		FVector Base = Cross.Z > 0 ? FVector::UpVector : FVector::DownVector;
		int BaseIndex = NumTriangles * 3;
		InstanceIDs[BaseIndex + 0] = Builder->AppendInstance(VertexIDs[LeftStart]);
		InstanceIDs[BaseIndex + 1] = Builder->AppendInstance(VertexIDs[LeftCurve.Points.Num() + RightStart]);
		InstanceIDs[BaseIndex + 2] = Builder->AppendInstance(VertexIDs[LeftSide ? LeftStart + 1 : LeftCurve.Points.Num() + RightStart + 1]);
		Builder->SetInstanceNormal(InstanceIDs[BaseIndex + 0], LeftCurve.GetNormal(LeftStart, Base));
		Builder->SetInstanceNormal(InstanceIDs[BaseIndex + 1], RightCurve.GetNormal(RightStart, Base));
		Builder->SetInstanceNormal(InstanceIDs[BaseIndex + 2], LeftSide ? LeftCurve.GetNormal(LeftStart + 1, Base) : RightCurve.GetNormal(RightStart + 1, Base));
		Builder->SetInstanceUV(InstanceIDs[BaseIndex + 0], FVector2D(P0) * UVScale, 0);
		Builder->SetInstanceUV(InstanceIDs[BaseIndex + 1], FVector2D(P1) * UVScale, 0);
		Builder->SetInstanceUV(InstanceIDs[BaseIndex + 2], FVector2D(P2) * UVScale, 0);
		Builder->AppendTriangle(InstanceIDs[BaseIndex + 0], InstanceIDs[BaseIndex + 1], InstanceIDs[BaseIndex + 2], Group);
		NumTriangles++;
	};
	BuildStrip(LeftCurve, RightCurve, AddTriangle);
}

void FStaticRoadMesh::AddPolygon(UMaterialInterface* SurfaceMaterial, UMaterialInterface* BackfaceMaterial, const TArray<FVector>& Points)
{
	auto cdt = CDT::Triangulation<double>(CDT::VertexInsertionOrder::AsProvided, CDT::IntersectingConstraintEdges::Resolve, 0);
	std::vector<CDT::V2d<double>> verts;
	std::vector<CDT::Edge> edges;
	CDT::TriIndUSet toErase;
	for (int i = 0; i < Points.Num(); i++)
	{
		edges.push_back(CDT::Edge(i, (i + 1) % Points.Num()));
		verts.push_back(CDT::V2d<double>::make(Points[i].X, Points[i].Y));
	}
	cdt.insertVertices(verts);
	cdt.insertEdges(edges);
	cdt.refineTriangles(Points.Num() * 4, toErase);
	cdt.eraseOuterTrianglesAndHoles();
	TArray<FVector> Verts;
	TArray<FIndex3i> Triangles;
	TArray<FIndex3i> InvTriangles;
	TArray<int> TrianglesToSolve;
	TMap<int, TArray<int>> VertexNeighbors;
	Verts.AddUninitialized(cdt.vertices.size());
	Triangles.AddUninitialized(cdt.triangles.size());
	InvTriangles.AddUninitialized(cdt.triangles.size());
	TrianglesToSolve.AddUninitialized(cdt.triangles.size());
	for (int i = 0; i < cdt.vertices.size(); i++)
		Verts[i] = FVector(cdt.vertices[i].x, cdt.vertices[i].y, i < Points.Num() ? Points[i].Z : 0);
	for (int i = 0; i < cdt.triangles.size(); i++)
	{
		FIndex3i& Triangle = Triangles[i];
		FIndex3i& InvTriangle = InvTriangles[i];
		Triangle = FIndex3i(cdt.triangles[i].vertices[0], cdt.triangles[i].vertices[1], cdt.triangles[i].vertices[2]);
		InvTriangle = FIndex3i(cdt.triangles[i].vertices[0], cdt.triangles[i].vertices[2], cdt.triangles[i].vertices[1]);
		TrianglesToSolve[i] = i;
		for (int j = 0; j < 3; j++)
			for (int k = 1; k < 3; k++)
				VertexNeighbors.FindOrAdd(Triangle[j]).AddUnique(Triangle[(j + k) % 3]);
	}
	TSet<int> BoundaryVertices;
	TSet<int> InnerVertices;
	for (std::pair<const CDT::Edge, CDT::EdgeVec>& pair : cdt.pieceToOriginals)
	{
		if (!pair.second.size())
			continue;
		CDT::Edge& edge = pair.second[0];
		FVector& P1 = Verts[edge.v1()];
		FVector& P2 = Verts[edge.v2()];
		double Distance = FVector2D::Distance((FVector2D&)P1, (FVector2D&)P2);
		auto Solve = [&](int Index)
		{
			if (Index >= Points.Num() && !BoundaryVertices.Contains(Index))
			{
				FVector& P = Verts[Index];
				P.Z = FMath::Lerp(P1.Z, P2.Z, FVector2D::Distance((FVector2D&)P, (FVector2D&)P1) / Distance);
				BoundaryVertices.Add(Index);
			}
		};
		Solve(pair.first.v1());
		Solve(pair.first.v2());
	}
	while (TrianglesToSolve.Num())
	{
		auto GetVerticesToSolve = [&](FIndex3i& Triangle)->TArray<int>
		{
			TArray<int> Vertices;
			for (int i = 0; i < 3; i++)
				if (Triangle[i] >= Points.Num() && !BoundaryVertices.Contains(Triangle[i]) && !InnerVertices.Contains(Triangle[i]))
					Vertices.Add(i);
			return MoveTemp(Vertices);
		};
		for (int i = 0; i < TrianglesToSolve.Num();)
		{
			FIndex3i& Triangle = Triangles[TrianglesToSolve[i]];
			TArray<int> Vertices = GetVerticesToSolve(Triangle);
			if (Vertices.Num() > 1)
				i++;
			else
			{
				if (Vertices.Num())
				{
					int j = Vertices[0];
					FVector& P0 = Verts[Triangle[j]];
					FVector& P1 = Verts[Triangle[(j + 1) % 3]];
					FVector& P2 = Verts[Triangle[(j + 2) % 3]];
					double L1 = FVector2D::Distance((FVector2D&)P0, (FVector2D&)P1);
					double L2 = FVector2D::Distance((FVector2D&)P0, (FVector2D&)P2);
					P0.Z = FMath::Lerp(P1.Z, P2.Z, L1 / (L1 + L2));
					InnerVertices.Add(Triangle[j]);
				}
				TrianglesToSolve.RemoveAt(i);
			}
		}
	}
	int NumSmoothIteration = 8;
	for (int i = 0; i < NumSmoothIteration; i++)
	{
		TMap<int, double> Heights;
		for (int Index : InnerVertices)
		{
			TArray<int>& Neighbors = VertexNeighbors[Index];
			double H = 0;
			for (int Neighbor : Neighbors)
				H += Verts[Neighbor].Z / Neighbors.Num();
			Heights.Add(Index, H);
		}
		for (int Index : InnerVertices)
			Verts[Index].Z = Heights[Index];
	}
	if (SurfaceMaterial)
		AddTriangles(SurfaceMaterial, InvTriangles, Verts, FVector::UpVector);
	if (BackfaceMaterial)
		AddTriangles(BackfaceMaterial, Triangles, Verts, FVector::DownVector);
}

void FStaticRoadMesh::AddPolygons(UMaterialInterface* Material, const TArray<FVector>& Positions, const TArray<FVector2D>& UVs, int NumCols, int NumRows)
{
	FPolygonGroupID Group = GetGroupID(Material);
	TArray<FVertexID> VertexIDs;
	VertexIDs.AddUninitialized(Positions.Num());
	for (int i = 0; i < Positions.Num(); i++)
		VertexIDs[i] = Builder->AppendVertex(Positions[i]);
	TArray<FVertexInstanceID> InstanceIDs;
	InstanceIDs.AddUninitialized(NumCols * NumRows * 6);
	int Stride = NumCols + 1;
	for (int i = 0; i < NumRows; i++)
	{
		for (int j = 0; j < NumCols; j++)
		{
			int Indices[6] = { 0, Stride, 1, 1, Stride, Stride + 1 };
			int BaseIndex = (i * NumCols + j) * 6;
			int BaseVertex = i * Stride + j;
			for (int k = 0; k < 6; k += 3)
			{
				FVector X = (Positions[BaseVertex + Indices[k + 2]] - Positions[BaseVertex + Indices[k]]).GetSafeNormal();
				FVector Y = (Positions[BaseVertex + Indices[k + 1]] - Positions[BaseVertex + Indices[k]]).GetSafeNormal();
				FVector Normal = (X ^ Y).GetSafeNormal();
				for (int l = 0; l < 3; l++)
				{
					int Vertex = BaseVertex + Indices[k + l];
					int Index = BaseIndex + k + l;
					InstanceIDs[Index] = Builder->AppendInstance(VertexIDs[Vertex]);
					Builder->SetInstanceNormal(InstanceIDs[Index], Normal);
					Builder->SetInstanceUV(InstanceIDs[Index], UVs[Vertex], 0);
				}
				Builder->AppendTriangle(InstanceIDs[BaseIndex + k], InstanceIDs[BaseIndex + k + 1], InstanceIDs[BaseIndex + k + 2], Group);
			}
		}
	}
}

void FStaticRoadMesh::AddTriangles(UMaterialInterface* Material, const TArray<FIndex3i>& Triangles, const TArray<FVector>& Vertices, const FVector& Normal)
{
	FPolygonGroupID Group = GetGroupID(Material);
	TArray<FVertexID> VertexIDs;
	VertexIDs.AddUninitialized(Vertices.Num());
	for (int i = 0; i < Vertices.Num(); i++)
		VertexIDs[i] = Builder->AppendVertex(Vertices[i]);
	TArray<FVertexInstanceID> InstanceIDs;
	InstanceIDs.AddUninitialized(Triangles.Num()*3);
	double UVScale = GetMutableDefault<USettings_Global>()->UVScale;
	for (int i = 0; i < Triangles.Num(); i++)
	{
		for (int j = 0; j < 3; j++)
		{
			int Index = i * 3 + j;
			InstanceIDs[Index] = Builder->AppendInstance(VertexIDs[Triangles[i][j]]);
			Builder->SetInstanceNormal(InstanceIDs[Index], Normal);
			Builder->SetInstanceUV(InstanceIDs[Index], FVector2D(Vertices[Triangles[i][j]]) * UVScale, 0);
		}
		Builder->AppendTriangle(InstanceIDs[i * 3 + 0], InstanceIDs[i * 3 + 1], InstanceIDs[i * 3 + 2], Group);
	}
}

void FStaticRoadMesh::Build(USceneComponent* Component)
{
	SCOPE_CYCLE_COUNTER(STAT_BuildMesh);
	Cast<UStaticMeshComponent>(Component)->SetStaticMesh(CreateMesh(Component->GetOwner()));
}

void FProcRoadMesh::AddStrip(UMaterialInterface* Material, const FPolyline& LeftCurve, const FPolyline& RightCurve)
{
	SCOPE_CYCLE_COUNTER(STAT_AddStrip);
	FProcMeshSection& Section = Sections.FindOrAdd(Material);
	int BaseVertex = Section.ProcVertexBuffer.Num();
	Section.ProcVertexBuffer.AddUninitialized(LeftCurve.Points.Num() + RightCurve.Points.Num());
	double& UVScale = GetMutableDefault<USettings_Global>()->UVScale;
	for (int i = 0; i < LeftCurve.Points.Num(); i++)
	{
		FProcMeshVertex& Vertex = Section.ProcVertexBuffer[BaseVertex + i];
		Vertex.Position = LeftCurve.Points[i].Pos;
		Vertex.Tangent = FProcMeshTangent(LeftCurve.GetDir(i), false);
		Vertex.Normal = LeftCurve.GetNormal(i, FVector::UpVector);
		Vertex.UV0 = LeftCurve.Points[i].Pos2D() * UVScale;
		Section.SectionLocalBox += Vertex.Position;
	}
	for (int i = 0; i < RightCurve.Points.Num(); i++)
	{
		FProcMeshVertex& Vertex = Section.ProcVertexBuffer[BaseVertex + LeftCurve.Points.Num() + i];
		Vertex.Position = RightCurve.Points[i].Pos;
		Vertex.Tangent = FProcMeshTangent(RightCurve.GetDir(i), false);
		Vertex.Normal = RightCurve.GetNormal(i, FVector::UpVector);
		Vertex.UV0 = RightCurve.Points[i].Pos2D() * UVScale;
		Section.SectionLocalBox += Vertex.Position;
	}
	int BaseIndex = Section.ProcIndexBuffer.Num();
	Section.ProcIndexBuffer.AddUninitialized((LeftCurve.Points.Num() + RightCurve.Points.Num() - 2) * 3);
	int NumTriangles = 0;
	auto AddTriangle = [&](int LeftStart, int RightStart, bool LeftSide)
	{
		int Base = BaseIndex + NumTriangles * 3;
		Section.ProcIndexBuffer[Base + 0] = BaseVertex + LeftStart;
		Section.ProcIndexBuffer[Base + 1] = BaseVertex + LeftCurve.Points.Num() + RightStart;
		Section.ProcIndexBuffer[Base + 2] = BaseVertex + (LeftSide ? (LeftStart + 1) : (LeftCurve.Points.Num() + RightStart + 1));
		NumTriangles++;
	};
	BuildStrip(LeftCurve, RightCurve, AddTriangle);
}

void FProcRoadMesh::AddPolygons(UMaterialInterface* Material, const TArray<FVector>& Positions, const TArray<FVector2D>& UVs, int NumCols, int NumRows)
{

}

void FProcRoadMesh::AddTriangles(UMaterialInterface* Material, const TArray<FIndex3i>& Triangles, const TArray<FVector>& Vertices)
{
	FProcMeshSection& Section = Sections.FindOrAdd(Material);
	int BaseVertex = Section.ProcVertexBuffer.Num();
	Section.ProcVertexBuffer.AddUninitialized(Vertices.Num());
	for (int i = 0; i < Vertices.Num(); i++)
	{
		FProcMeshVertex& Vertex = Section.ProcVertexBuffer[BaseVertex + i];
		Vertex.Position = Vertices[i];
		Vertex.Tangent = FProcMeshTangent(FVector::ForwardVector, false);
		Vertex.Normal = FVector::UpVector;
		Vertex.UV0 = FVector2D(Vertices[i]) * 0.01;
		Section.SectionLocalBox += Vertex.Position;
	}
	int BaseIndex = Section.ProcIndexBuffer.Num();
	Section.ProcIndexBuffer.AddUninitialized(Triangles.Num() * 3);
	for (int i = 0; i < Triangles.Num(); i++)
		for (int j = 0; j < 3; j++)
			Section.ProcIndexBuffer[BaseIndex + i*3+j] = BaseVertex + Triangles[i][j];
}

void FProcRoadMesh::Build(USceneComponent* Component)
{
	SCOPE_CYCLE_COUNTER(STAT_BuildMesh);
	UProceduralMeshComponent* ProcComponent = Cast<UProceduralMeshComponent>(Component);
	int Index = 0;
	for (auto It = Sections.CreateIterator(); It; ++It)
	{
		It->Value.bEnableCollision = true;
		ProcComponent->SetMaterial(Index, It->Key);
		ProcComponent->SetProcMeshSection(Index, It->Value);
		Index++;
	}
}

void FInstanceBuilder::AttachToActor(AActor* Actor)
{
	for (auto KV : Instances)
	{
		UInstancedStaticMeshComponent* Component = KV.Value.bForceISM ? NewObject<UInstancedStaticMeshComponent>(Actor) : NewObject<UHierarchicalInstancedStaticMeshComponent>(Actor);
		Component->SetReceivesDecals(false);
		Component->SetStaticMesh(KV.Key);
		Component->SetCullDistances(0, 0);
		for (FTransform& Instance : KV.Value.Instances)
			Component->AddInstance(Instance);
		Component->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		Actor->AddInstanceComponent(Component);
		Component->RegisterComponent();
	}
}

void FDecalBuilder::AttachToActor(AActor* Actor)
{
	FTransform LocalRot(FRotator(-90, 0, 0));
	for (FDecal& Decal : Decals)
	{
		UDecalComponent* Component = NewObject<UDecalComponent>(Actor);
		Component->SetRelativeTransform(LocalRot * Decal.Trans);
		Component->SetDecalMaterial(Decal.Material);
		Component->DecalSize = FVector(Decal.Size.X / 2, Decal.Size.Y, Decal.Size.X);
	//  Component->SetFadeScreenSize(0.05);
		Component->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		Actor->AddInstanceComponent(Component);
		Component->RegisterComponent();
	}
}