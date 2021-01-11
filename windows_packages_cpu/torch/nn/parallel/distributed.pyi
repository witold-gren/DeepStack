from ..modules import Module
from typing import Any, Optional
from .common_types import _devices_t, _device_t


class DistributedDataParallel(Module):
    process_group: Any = ...
    dim: int = ...
    module: Module = ...
    device_ids: _devices_t = ...
    output_device: _device_t = ...
    broadcast_buffers: bool = ...
    check_reduction: bool = ...
    broadcast_bucket_size: float = ...
    bucket_bytes_cap: float = ...

    # TODO type process_group once `distributed` module is stubbed
    def __init__(self, module: Module, device_ids: Optional[_devices_t] = ...,
                 output_device: Optional[_device_t] = ..., dim: int = ...,
                 broadcast_buffers: bool = ..., process_group: Optional[Any] = ..., bucket_cap_mb: float = ...,
                 check_reduction: bool = ...) -> None: ...
