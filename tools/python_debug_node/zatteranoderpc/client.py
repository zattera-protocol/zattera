"""
Zattera Node RPC Client

A simple WebSocket-based JSON-RPC client for communicating with zatterad nodes.
"""

from typing import Dict, Optional, Any
import logging

from websocket import create_connection, WebSocket
import json

logger = logging.getLogger(__name__)


class ZatteraNodeRPC:
    """WebSocket JSON-RPC client for Zattera nodes"""

    def __init__(self, url: str, username: str = "", password: str = "") -> None:
        """
        Initialize a connection to a Zattera node.

        Args:
            url: WebSocket URL (e.g., "ws://127.0.0.1:8095")
            username: Optional username for authentication
            password: Optional password for authentication
        """
        self.url = url
        self.username = username
        self.password = password
        self._ws: Optional[WebSocket] = None
        self._connect()

    def __enter__(self):
        """Context manager entry"""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.close()

    def __del__(self) -> None:
        """Cleanup WebSocket connection on deletion"""
        self.close()

    def call(self, request: Dict[str, Any]) -> Any:
        """
        Execute a JSON-RPC request.

        Args:
            request: JSON-RPC request dictionary with keys:
                - jsonrpc: "2.0"
                - method: "call"
                - params: [api_id, method_name, args]
                - id: request ID

        Returns:
            The result from the JSON-RPC response

        Raises:
            Exception: If the RPC call returns an error
        """
        self._ensure_connected()

        try:
            request_str = json.dumps(request)
            logger.debug(f"Sending RPC request: {request_str}")
            self._ws.send(request_str)

            response_str = self._ws.recv()
            logger.debug(f"Received RPC response: {response_str}")
            response = json.loads(response_str)

            if "error" in response:
                error = response["error"]
                error_msg = f"RPC Error: {error.get('message', 'Unknown error')}"
                if "data" in error:
                    error_msg += f" - Data: {error['data']}"
                logger.error(error_msg)
                raise Exception(error_msg)

            return response.get("result")

        except Exception as e:
            logger.error(f"RPC execution failed: {e}")
            raise

    def close(self) -> None:
        """Close the WebSocket connection"""
        if self._ws:
            self._ws.close()
            self._ws = None
            logger.info("Closed connection to Zattera node")

    def _connect(self) -> None:
        """Establish WebSocket connection to the node"""
        try:
            self._ws = create_connection(self.url)
            logger.info(f"Connected to Zattera node at {self.url}")
        except Exception as e:
            logger.error(f"Failed to connect to {self.url}: {e}")
            raise

    def _ensure_connected(self) -> None:
        """Ensure the WebSocket connection is active"""
        if self._ws is None or not self._ws.connected:
            logger.warning("WebSocket disconnected, reconnecting...")
            self._connect()
