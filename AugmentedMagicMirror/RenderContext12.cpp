// RenderContext.cpp : A context for rendering to a particular output Window
//

#include "stdafx.h"
#ifdef USE_D3DX12
#include "RenderContext12.h"

#include "GraphicsContext12.h"
#include "RenderingContext12.h"
#include "Mesh12.h"

#include "Camera.h"

namespace D3DX12
{
	RenderContext::RenderContext(_In_ GraphicsContext & DeviceContext, _In_ Window & TargetWindow, _In_ Camera & NoseCamera, _In_ Camera & LeftEyeCamera, _In_ Camera & RighEyeCamera)
		: ::RenderContext(TargetWindow, NoseCamera, LeftEyeCamera, RighEyeCamera)
		, DeviceContext(DeviceContext)
		, Viewport(), ScissorRect()
		, RTVDescSize(0), BufferFrameIndex(0)
	{
	}

	void RenderContext::Initialize()
	{
		if (TargetWindow.GetHandle() == nullptr)
		{
			Utility::Throw(L"Window Handle is invalid!");
		}

		TargetWindow.WindowResized += std::make_pair(this, &RenderContext::OnWindowSizeChange);

		CreateSwapChain();
		CreateRTVHeap();
		CreateDSVHeap();
		CreateDepthStencilView();
		InitializeRenderTargets();

		CreateCommandList();

		const Window::WindowSize & WindowSize = TargetWindow.GetWindowSize();
		NoseCamera.UpdateCamera(WindowSize);
		UpdateViewportAndScissorRect(WindowSize);

		Fence.Initialize(DeviceContext.GetDevice());
	}

	void RenderContext::Render(MeshList DrawCalls)
	{
		Render({ { DeviceContext.GetDefaultShader(), DrawCalls } });
	}

	void RenderContext::CreateSwapChain()
	{
		DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
		SwapChainDesc.BufferCount = BufferFrameCount;
		SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		SwapChainDesc.SampleDesc.Count = 1;

		Window::WindowSize Size = TargetWindow.GetWindowSize();
		SwapChainDesc.Width = Size.first;
		SwapChainDesc.Height = Size.second;

		Microsoft::WRL::ComPtr<IDXGISwapChain1> HelperSwapChain;
		Utility::ThrowOnFail(DeviceContext.GetFactory()->CreateSwapChainForHwnd(DeviceContext.GetCommandQueue().Get(), TargetWindow.GetHandle(), &SwapChainDesc, nullptr, nullptr, &HelperSwapChain));

		Utility::ThrowOnFail(HelperSwapChain.As(&SwapChain));
		BufferFrameIndex = SwapChain->GetCurrentBackBufferIndex();
	}

	void RenderContext::CreateRTVHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc = {};
		RTVHeapDesc.NumDescriptors = BufferFrameCount;
		RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		Utility::ThrowOnFail(DeviceContext.GetDevice()->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(&RTVHeap)));

		RTVDescSize = DeviceContext.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	void RenderContext::CreateDSVHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC DSVHeapDesc = {};
		DSVHeapDesc.NumDescriptors = 1;
		DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		DSVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		Utility::ThrowOnFail(DeviceContext.GetDevice()->CreateDescriptorHeap(&DSVHeapDesc, IID_PPV_ARGS(&DSVHeap)));
	}

	void RenderContext::CreateDepthStencilView()
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc = {};
		DepthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
		DepthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		DepthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

		D3D12_CLEAR_VALUE DSVClearValue = {};
		DSVClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		DSVClearValue.DepthStencil.Depth = 1.0f;
		DSVClearValue.DepthStencil.Stencil = 0;

		Window::WindowSize Size = TargetWindow.GetWindowSize();
		CD3DX12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, Size.first, Size.second, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		Utility::ThrowOnFail(DeviceContext.GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &DSVClearValue, IID_PPV_ARGS(&DepthStencelView)));

		DeviceContext.GetDevice()->CreateDepthStencilView(DepthStencelView.Get(), &DepthStencilViewDesc, DSVHeap->GetCPUDescriptorHandleForHeapStart());
	}

	void RenderContext::InitializeRenderTargets()
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHandle(RTVHeap->GetCPUDescriptorHandleForHeapStart());

		for (UINT i = 0; i < BufferFrameCount; ++i)
		{
			RenderTargets[i].Initialize(i, DeviceContext.GetDevice(), SwapChain, RTVHandle);
			RTVHandle.Offset(RTVDescSize);
		}
	}

	void RenderContext::CreateCommandList()
	{
		Utility::ThrowOnFail(DeviceContext.GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, RenderTargets[BufferFrameIndex].GetCommandAllocator().Get(), nullptr, IID_PPV_ARGS(&CommandList)));
		Utility::ThrowOnFail(CommandList->Close());
	}

	void RenderContext::ResizeBuffers(_In_ const Window::WindowSize & NewSize)
	{
		ReleaseSizeDependentBuffers();
		ResizeSwapChain(NewSize);
		RecreateSizeDependentBuffers();
	}

	void RenderContext::ReleaseSizeDependentBuffers()
	{
		for (UINT i = 0; i < BufferFrameCount; ++i)
		{
			RenderTargets[i].ReleaseWindowSizeDependendResources();
		}

		DepthStencelView.Reset();
	}

	void RenderContext::ResizeSwapChain(_In_ const Window::WindowSize & NewSize)
	{
		DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
		SwapChain->GetDesc(&SwapChainDesc);
		Utility::ThrowOnFail(SwapChain->ResizeBuffers(BufferFrameCount, NewSize.first, NewSize.second, SwapChainDesc.BufferDesc.Format, SwapChainDesc.Flags));

		BufferFrameIndex = SwapChain->GetCurrentBackBufferIndex();
	}

	void RenderContext::RecreateSizeDependentBuffers()
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHandle(RTVHeap->GetCPUDescriptorHandleForHeapStart());

		for (UINT i = 0; i < BufferFrameCount; ++i)
		{
			RenderTargets[i].CreateWindowSizeDependendResources(i, DeviceContext.GetDevice(), SwapChain, RTVHandle);
			RTVHandle.Offset(RTVDescSize);
		}

		CreateDepthStencilView();
	}

	void RenderContext::UpdateViewportAndScissorRect(_In_ const Window::WindowSize & Size)
	{
		Viewport.Width = static_cast<float>(Size.first);
		Viewport.Height = static_cast<float>(Size.second);
		Viewport.MaxDepth = 1.0f;

		ScissorRect.right = static_cast<LONG>(Size.first);
		ScissorRect.bottom = static_cast<LONG>(Size.second);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE RenderContext::GetRTVCPUHandle()
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(RTVHeap->GetCPUDescriptorHandleForHeapStart(), BufferFrameIndex, RTVDescSize);
	}

	void RenderContext::Render(_In_ RenderParameterList DrawCalls)
	{
		RenderTarget & CurrentRenderTarget = RenderTargets[BufferFrameIndex];
		CurrentRenderTarget.BeginFrame(CommandList);

		CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHandle(GetRTVCPUHandle());
		CD3DX12_CPU_DESCRIPTOR_HANDLE DSVHandle(DSVHeap->GetCPUDescriptorHandleForHeapStart());

		CommandList->OMSetRenderTargets(1, &RTVHandle, FALSE, &DSVHandle);
		CommandList->ClearRenderTargetView(RTVHandle, BackgroundColor.data(), 0, nullptr);
		CommandList->ClearDepthStencilView(DSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		CommandList->RSSetViewports(1, &Viewport);
		CommandList->RSSetScissorRects(1, &ScissorRect);

		for (RenderParameter & RenderCommand : DrawCalls)
		{
			RenderCommand.first.Prepare(CommandList, NoseCamera);

			for (ObjectList & ObjectsToRender : RenderCommand.second)
			{
				const Mesh & Mesh12 = static_cast<const Mesh &>(ObjectsToRender.first);
				Mesh12.Render(CommandList, RenderCommand.first, ObjectsToRender.second);
			}
		}

		CurrentRenderTarget.EndFrame(CommandList, DeviceContext.GetCommandQueue());
		Utility::ThrowOnFail(SwapChain->Present(0, 0));
		BufferFrameIndex = SwapChain->GetCurrentBackBufferIndex();
	}

	void RenderContext::OnWindowSizeChange(_In_ const Window::WindowSize & NewSize)
	{
		Fence.SetAndWait(DeviceContext.GetCommandQueue());

		ResizeBuffers(NewSize);
		UpdateViewportAndScissorRect(NewSize);
		NoseCamera.UpdateCamera(NewSize);
	}
}
#endif
