#pragma once

#include "Transform.h"

class GraphicsContext;
class Camera;
class RenderingContext;

class Mesh
{
public:
	Mesh(_In_ GraphicsContext & DeviceContext);

	void Create();
	void Render(_In_ Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> & CommandList, _In_ RenderingContext & RenderingContext, _In_ const TransformList & Objects);

private:
	struct Vertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT4 Color;
	};

	GraphicsContext & DeviceContext;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView;

	UINT IndexCount;

	void UploadVertices(_In_ Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> & CommandList, _Out_ Microsoft::WRL::ComPtr<ID3D12Resource> & UploadResource);
	void UploadIndices(_In_ Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> & CommandList, _Out_ Microsoft::WRL::ComPtr<ID3D12Resource> & UploadResource);
	void UploadData(_In_ Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> & CommandList, _Out_ Microsoft::WRL::ComPtr<ID3D12Resource> & Resource, _Out_ Microsoft::WRL::ComPtr<ID3D12Resource> & UploadResource, _In_reads_bytes_(DataSize) const void * Data, _In_ size_t  DataSize);

};
