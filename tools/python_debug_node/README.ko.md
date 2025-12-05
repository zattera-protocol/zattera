# Python Debug Node

## 개요

Python Debug Node는 실행 중인 Zattera Debug Node의 생성 및 유지 관리를 자동화하는 래퍼 클래스입니다. Debug Node는 Zattera의 플러그인으로, 로컬에 저장된 블록체인을 손상시키거나 라이브 체인에 변경 사항을 전파하지 않고 실제 세계 동작을 모방하는 방식으로 체인 상태를 실시간으로 로컬 수정할 수 있습니다.

debug node에 대한 자세한 정보는 [debug_node_plugin.md](debug-node-plugin.md)에서 찾을 수 있습니다.

## 왜 이것을 사용해야 하는가?

Debug Node Plugin은 강력하지만 노드 구성 및 RPC/WebSocket 인터페이스에 대한 지식이 필요합니다. 이 Python 모듈은 다음을 제공하여 프로세스를 단순화합니다:

- 프로그래밍 방식의 노드 시작 및 생명주기 관리
- 간단한 WebSocket 기반 JSON-RPC 클라이언트 (`zatteranoderpc`)
- 자동 정리를 위한 깔끔한 컨텍스트 관리자 (`with` 문) 인터페이스

## 어떻게 사용하는가?

### 설치

`tools/python_debug_node`로 이동하여 실행하십시오:

```bash
python3 setup.py install
```

### 사용법

스크립트에서 debug node를 사용하려면:

```python
from zatteradebugnode import ZatteraDebugNode
```

모듈은 두 가지 주요 구성 요소를 제공합니다:
- **`ZatteraDebugNode`** - debug node 인스턴스를 시작하고 관리하는 래퍼 클래스
- **`ZatteraNodeRPC`** - zatterad 노드와 통신하기 위한 WebSocket 기반 JSON-RPC 클라이언트 (`zatteranoderpc`에서 가져옴)

## 예제 스크립트

### debug_hardforks.py

[tests/debug_hardforks.py](tests/debug_hardforks.py) - 이 스크립트는 debug node를 사용한 하드포크 테스트를 시연합니다:
- debug node 시작
- 기존 체인에서 블록 재생
- `debug_set_hardfork()`를 사용하여 하드포크 예약
- 하드포크 후 블록 생성
- JSON-RPC 인터페이스를 통해 결과 분석 (예: 블록 생산자의 히스토그램 생성)

목적은 하드포크가 체인을 충돌시키지 않고 올바르게 실행되는지 확인하는 것입니다.

### debugnode.py

[zatteradebugnode/debugnode.py](zatteradebugnode/debugnode.py#L280) - 메인 모듈 파일에는 독립 실행형 예제가 포함되어 있습니다 (`python -m zatteradebugnode.debugnode`로 실행). 이 간단한 스크립트는:
- zatterad 경로 및 데이터 디렉토리에 대한 명령줄 인수 파싱
- debug node 생성 및 시작
- 블록체인 재생
- RPC 호출 또는 CLI wallet을 통한 사용자 상호 작용을 위해 무한정 대기
- SIGINT (Ctrl+C)시 안전하게 종료

### 기본 사용 패턴

이러한 스크립트의 중요한 부분은 다음과 같습니다:

```python
debug_node = ZatteraDebugNode(str(zatterad_path), str(data_dir))
with debug_node:
   # 블록체인으로 작업 수행
```

모듈은 `with` 구조를 사용하도록 설정되어 있습니다. 객체 생성은 단순히 올바른 디렉토리를 확인하고 내부 상태를 설정합니다. `with debug_node:`는 노드를 연결하고 내부 RPC 연결을 설정합니다. 그런 다음 스크립트는 원하는 모든 작업을 수행할 수 있습니다. `with` 블록이 끝나면 노드가 자동으로 종료되고 정리됩니다.

노드는 실행 중인 노드의 작업 데이터 디렉토리로 표준 Python `TemporaryDirectory`를 통해 시스템 표준 temp 디렉토리를 사용합니다. 스크립트가 수행해야 하는 유일한 작업은 zatterad 바이너리 위치와 채워진 데이터 디렉토리를 지정하는 것입니다. 대부분의 구성에서 이것은 Zattera의 git 루트 디렉토리에서 각각 `programs/zatterad/zatterad` 및 `witness_node_data_dir`입니다.

## API 레퍼런스

### ZatteraDebugNode 클래스

#### 생성자

```python
ZatteraDebugNode(
    zatterad_path: str,
    data_dir: str,
    zatterad_args: str = "",
    plugins: List[str] = [],
    apis: List[str] = [],
    zatterad_stdout: Optional[IO] = None,
    zatterad_stderr: Optional[IO] = None
)
```

**매개변수:**
- `zatterad_path` - zatterad 바이너리 경로
- `data_dir` - 기존 데이터 디렉토리 경로 (블록체인 데이터 소스로 사용)
- `zatterad_args` - zatterad에 전달할 추가 명령줄 인수
- `plugins` - 추가로 활성화할 플러그인 (`witness` 및 `debug_node` 외에)
- `apis` - 추가로 노출할 API (`database_api`, `login_api`, `debug_node_api` 외에)
- `zatterad_stdout` - zatterad stdout용 스트림 (기본값: `/dev/null`)
- `zatterad_stderr` - zatterad stderr용 스트림 (기본값: `/dev/null`)

#### Debug Node API 메서드

**`debug_generate_blocks(count: int) -> int`**

체인에 `count`개의 새 블록을 생성합니다. 대기 중인 트랜잭션이 적용되며, 그렇지 않으면 블록이 비어 있습니다. 디버그 키 `5JHNbFNDg834SFj8CMArV6YW7td4zrPzXveqTfaShmYVuYNeK69`(`get_dev_key zattera debug`에서 생성)를 사용합니다. **라이브 체인에서는 절대 이 키를 사용하지 마십시오.**

실제로 생성된 블록 수를 반환합니다.

**`debug_generate_blocks_until(timestamp: int, generate_sparsely: bool = True) -> Tuple[str, int]`**

헤드 블록 시간이 지정된 POSIX 타임스탬프에 도달할 때까지 블록을 생성합니다.

- `timestamp` - 목표 헤드 블록 시간 (POSIX 타임스탬프)
- `generate_sparsely` - `True`이면 중간 블록을 건너뛰고 헤드 블록 시간만 업데이트합니다. 모든 블록을 생성하지 않고 시간 기반 이벤트(지급, 대역폭 업데이트)를 트리거하는 데 유용합니다. `False`이면 모든 블록을 순차적으로 생성합니다.

(새로운_헤드_블록_시간, 생성된_블록_수)의 튜플을 반환합니다.

**`debug_set_hardfork(hardfork_id: int) -> None`**

다음 블록에서 실행될 하드포크를 예약합니다. 트리거하려면 `debug_generate_blocks(1)`을 호출하십시오. ID가 `hardfork_id` 이하인 모든 하드포크가 예약됩니다.

- `hardfork_id` - 하드포크 ID (1부터 시작; 0은 제네시스). 최대값은 [hardfork.d/0-preamble.hf](../../src/core/protocol/hardfork.d/0-preamble.hf)의 `ZATTERA_NUM_HARDFORKS`로 정의됩니다.

**`debug_has_hardfork(hardfork_id: int) -> bool`**

하드포크가 적용되었는지 확인합니다.

**`debug_get_witness_schedule() -> dict`**

현재 증인 스케줄을 반환합니다.

**`debug_get_hardfork_property_object() -> dict`**

현재 하드포크 상태가 포함된 하드포크 속성 객체를 반환합니다.

### ZatteraNodeRPC 클래스

zatterad 노드와 통신하기 위한 간단한 WebSocket 기반 JSON-RPC 클라이언트입니다.

#### 생성자

```python
ZatteraNodeRPC(url: str, username: str = "", password: str = "")
```

**매개변수:**
- `url` - WebSocket URL (예: `"ws://127.0.0.1:8095"`)
- `username` - 인증을 위한 선택적 사용자 이름
- `password` - 인증을 위한 선택적 비밀번호

#### 메서드

**`call(request: Dict[str, Any]) -> Any`**

JSON-RPC 요청을 실행합니다.

**요청 형식:**
```python
{
    "jsonrpc": "2.0",
    "method": "call",
    "params": [api_id, method_name, args],
    "id": 1
}
```

**API ID:**
- `0` - `database_api`
- `1` - `login_api`
- `2` - `debug_node_api`
- `3+` - `ZatteraDebugNode` 생성자에서 지정된 순서대로 추가 API

JSON-RPC 응답의 `result` 필드를 반환합니다.

**예제:**
```python
# 동적 글로벌 속성 가져오기
request = {
    "jsonrpc": "2.0",
    "method": "call",
    "params": [0, "get_dynamic_global_properties", []],
    "id": 1
}
result = rpc.call(request)
```

**`close() -> None`**

WebSocket 연결을 닫습니다. 컨텍스트 관리자(`with` 문) 사용 시 자동으로 호출됩니다.

## 구현 노트

### 현재 아키텍처

모듈은 다음 구성 요소로 자체 포함되어 있습니다:

- **`zatteradebugnode.ZatteraDebugNode`** - debug node 관리를 위한 메인 래퍼
- **`zatteranoderpc.ZatteraNodeRPC`** - 커스텀 WebSocket JSON-RPC 클라이언트
- WebSocket 연결을 위해 `websocket-client` 라이브러리 사용

### 구성

debug node는 다음으로 자동 구성됩니다:
- **P2P 엔드포인트**: `127.0.0.1:2001` (원격 연결 방지를 위해 localhost만)
- **RPC 엔드포인트**: `127.0.0.1:8095` (보안을 위해 localhost만)
- **활성화된 플러그인**: `witness`, `debug_node`, 생성자에서 지정된 추가 플러그인
- **공개 API**: `database_api`, `login_api`, `debug_node_api`, 생성자에서 지정된 추가 API

### 데이터 디렉토리 처리

`ZatteraDebugNode`는 Python의 `TemporaryDirectory`를 사용하여 임시 작업 디렉토리를 생성합니다:
1. 소스 데이터 디렉토리에서 모든 디렉토리 복사
2. `db_version` 파일이 있으면 복사
3. 커스텀 `config.ini` 생성
4. 종료 시 자동 정리

소스 데이터 디렉토리는 변경되지 않고 재사용할 수 있습니다.

## 향후 개선 사항

잠재적 개선 영역:

- **상위 수준 API 래퍼:** 일반적인 database_api 및 condenser_api 호출을 위한 Python 메서드 제공
- **코드 생성:** C++ 소스 또는 JSON 스키마에서 API 래퍼 자동 생성
- **테스트 프레임워크:** 구조화된 통합 테스트를 위해 pytest 또는 unittest와 통합
- **체인 상태 스냅샷:** 재현 가능한 테스트를 위한 체인 상태 저장/로드 지원
- **다중 노드 지원:** P2P 테스트 시나리오를 위한 여러 debug node 실행 기능
